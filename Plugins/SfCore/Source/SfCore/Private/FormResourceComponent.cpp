﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "FormResourceComponent.h"

#include "FormCharacterComponent.h"
#include "FormStatComponent.h"
#include "Net/UnrealNetwork.h"

FResource::FResource(): Value(0), MaxValueOverride(0)
{
}

bool FResource::operator==(const FResource& Other) const
{
	if (Tag != Other.Tag) return false;
	if (Value != Other.Value) return false;
	return true;
}

bool FResource::operator!=(const FResource& Other) const
{
	return !(*this == Other);
}

FResourceArray::FResourceArray()
{
}

bool FResourceArray::operator==(const FResourceArray& Other) const
{
	return Items == Other.Items;
}

bool FResourceArray::operator!=(const FResourceArray& Other) const
{
	return !(*this == Other);
}

bool FResourceArray::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	TArray<float> Values;
	if (Ar.IsSaving())
	{
		Values.Reserve(Items.Num());
		for (FResource Resource : Items)
		{
			Values.Add(Resource.Value);
		}
	}
	Ar << Values;
	if (Ar.IsLoading())
	{
		while (Items.Num() > Values.Num())
		{
			Items.RemoveAt(Items.Num() - 1);
		}
		while (Values.Num() > Items.Num())
		{
			Items.Emplace();
		}
		for (uint8 i = 0; i < Items.Num(); i++)
		{
			Items[i].Value = Values[i];
		}
	}
	return bOutSuccess;
}

UFormResourceComponent::UFormResourceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bReplicateUsingRegisteredSubObjectList = true;
	SetIsReplicatedByDefault(true);
}

void UFormResourceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams ResourceParams;
	ResourceParams.bIsPushBased = true;
	ResourceParams.Condition = COND_None;
	//We handle owner replication for resources purely with the FormCharacterComponent if it is available.
	if (GetOwner() && GetOwner()->FindComponentByClass(UFormCharacterComponent::StaticClass()))
	{
		ResourceParams.Condition = COND_SkipOwner;
	}
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormResourceComponent, Resources, ResourceParams);
}

void UFormResourceComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GetOwner()->HasAuthority()) return;
	if (CalculateTimeUntilServerTimestamp(GetWorld(), NextReplicationServerTimestamp) < 0)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(UFormResourceComponent, Resources, this);
		NextReplicationServerTimestamp = CalculateFutureServerTimestamp(GetWorld(), CalculatedTimeToEachReplication);
	}
}

void UFormResourceComponent::SetupFormResource(UFormCoreComponent* InFormCore)
{
	FormCore = InFormCore;
	CalculatedTimeToEachResourceUpdate = 1.f / OwnerResourceUpdateFrequencyPerSecond;
	CalculatedTimeToEachReplication = 1.f / NonOwnerResourceReplicationFrequencyPerSecond;
}

void UFormResourceComponent::SecondarySetupFormResource()
{
	FormStat = FormCore->GetFormStat();
	Resources.Items = ResourcesToRegister;
	for (FResource& Resource : Resources.Items)
	{
		const float MaxValue = GetMaxValue(Resource);
		Resource.Value = FMath::Clamp(Resource.Value, 0, MaxValue);
	}
}

float UFormResourceComponent::GetResourceValue(const FGameplayTag InTag) const
{
	for (const FResource& Resource : Resources.Items)
	{
		if (Resource.Tag == InTag)
		{
			return Resource.Value;
		}
	}
	return 0;
}

FResource* UFormResourceComponent::GetResourceFromTag(const FGameplayTag& InTag)
{
	for (FResource& Resource : Resources.Items)
	{
		if (Resource.Tag == InTag)
		{
			return &Resource;
		}
	}
	return nullptr;
}

bool UFormResourceComponent::Server_AddResourceValue(const FGameplayTag InTag, const float InValue)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogSfCore, Error, TEXT("Server_AddResourceValue called on class UFormResourceComponent without authority."));
		return false;
	}
	FResource* FoundResource = GetResourceFromTag(InTag);
	if (FoundResource == nullptr) return false;
	LocalInternalAddResourceValue(*FoundResource, InValue);
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormResourceComponent, Resources, this);
	return true;
}

bool UFormResourceComponent::Server_RemoveResourceValue(const FGameplayTag InTag, const float InValue)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogSfCore, Error, TEXT("Server_RemoveResourceValue called on class UFormResourceComponent without authority."));
		return false;
	}
	FResource* FoundResource = GetResourceFromTag(InTag);
	if (FoundResource == nullptr) return false;
	LocalInternalRemoveResourceValue(*FoundResource, InValue);
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormResourceComponent, Resources, this);
	return true;
}

bool UFormResourceComponent::Server_SetResourceValue(const FGameplayTag InTag, const float InValue)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogSfCore, Error, TEXT("Server_SetResourceValue called on class UFormResourceComponent without authority."));
		return false;
	}
	FResource* FoundResource = GetResourceFromTag(InTag);
	if (FoundResource == nullptr) return false;
	LocalInternalSetResourceValue(*FoundResource, InValue);
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormResourceComponent, Resources, this);
	return true;
}

bool UFormResourceComponent::Predicted_AddResourceValue(const FGameplayTag InTag, const float InValue)
{
	FResource* FoundResource = GetResourceFromTag(InTag);
	if (FoundResource == nullptr) return false;
	LocalInternalAddResourceValue(*FoundResource, InValue);
	if (GetOwner()->HasAuthority())
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(UFormResourceComponent, Resources, this);
	}
	else
	{
		if (Client_OnResourceUpdate.IsBound())
		{
			Client_OnResourceUpdate.Broadcast();
		}
	}
	return true;
}

bool UFormResourceComponent::Predicted_RemoveResourceValue(const FGameplayTag InTag, const float InValue)
{
	FResource* FoundResource = GetResourceFromTag(InTag);
	if (FoundResource == nullptr) return false;
	LocalInternalRemoveResourceValue(*FoundResource, InValue);
	if (GetOwner()->HasAuthority())
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(UFormResourceComponent, Resources, this);
	}
	else
	{
		if (Client_OnResourceUpdate.IsBound())
		{
			Client_OnResourceUpdate.Broadcast();
		}
	}
	return true;
}

bool UFormResourceComponent::Predicted_SetResourceValue(const FGameplayTag InTag, const float InValue)
{
	FResource* FoundResource = GetResourceFromTag(InTag);
	if (FoundResource == nullptr) return false;
	LocalInternalSetResourceValue(*FoundResource, InValue);
	if (GetOwner()->HasAuthority())
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(UFormResourceComponent, Resources, this);
	}
	else
	{
		if (Client_OnResourceUpdate.IsBound())
		{
			Client_OnResourceUpdate.Broadcast();
		}
	}
	return true;
}

float UFormResourceComponent::GetMaxValue(const FResource& Resource) const
{
	if (!FormStat && Resource.MaxValueOverride == 0)
	{
		UE_LOG(LogSfCore, Error, TEXT("UFormResourceComponent class %s contains a resource that uses a max value stat, but does not have a UFormStatComponent."), *GetClass()->GetName());
		return 0;
	}
	return Resource.MaxValueOverride != 0 ? Resource.MaxValueOverride : FormStat->GetStat(Resource.MaxValueStat);
}

void UFormResourceComponent::LocalInternalAddResourceValue(FResource& Resource, const float InValue) const
{
	const float MaxValue = GetMaxValue(Resource);
	Resource.Value = FMath::Clamp(Resource.Value + InValue, 0, MaxValue);
}

void UFormResourceComponent::LocalInternalRemoveResourceValue(FResource& Resource, const float InValue) const
{
	const float MaxValue = GetMaxValue(Resource);
	Resource.Value = FMath::Clamp(Resource.Value - InValue, 0, MaxValue);
}

void UFormResourceComponent::LocalInternalSetResourceValue(FResource& Resource, const float InValue) const
{
	const float MaxValue = GetMaxValue(Resource);
	Resource.Value = FMath::Clamp(InValue, 0, MaxValue);
}

void UFormResourceComponent::OnRep_Resources()
{
	if (Client_OnResourceUpdate.IsBound())
	{
		Client_OnResourceUpdate.Broadcast();
	}
}
