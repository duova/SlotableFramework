// Fill out your copyright notice in the Description page of Project Settings.


#include "SfObject.h"

#include "FormCharacterComponent.h"

void USfObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//Get blueprint class replicated properties and make them push based.
	if (const UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass()))
	{
		TArray<FLifetimeProperty> BPLifetimeProps;
		BPClass->GetLifetimeBlueprintReplicationList(BPLifetimeProps);
		for (FLifetimeProperty Prop : BPLifetimeProps)
		{
			Prop.bIsPushBased = true;
			OutLifetimeProps.AddUnique(Prop);
		}
	}
}

bool USfObject::IsSupportedForNetworking() const
{
	return true;
}

int32 USfObject::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	checkf(GetOuter() != nullptr, TEXT("SfObject has no outer."));
	//Use the function callspace of the outer.
	return GetOuter()->GetFunctionCallspace(Function, Stack);
}

bool USfObject::CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack)
{
	checkf(!HasAnyFlags(RF_ClassDefaultObject), TEXT("Attempted to call remote function on CDO."));
	AActor* Owner = GetOwner();
	if (UNetDriver* NetDriver = Owner->GetNetDriver())
	{
		//Call using owing actor's NetDriver.
		NetDriver->ProcessRemoteFunction(Owner, Function, Parms, OutParms, Stack, this);
		return true;
	}
	return false;
}

void USfObject::Destroy()
{
	if (IsValid(this))
	{
		checkf(GetOwner()->HasAuthority() == true, TEXT("SfObject does not have authority to destroy itself."));
		MarkAsGarbage();
	}
}

bool USfObject::IsFormCharacter() const
{
	if (GetOwner()->FindComponentByClass(UFormCharacterComponent::StaticClass()))
	{
		return true;
	}
	return false;
}

UFormCharacterComponent* USfObject::GetFormCharacter() const
{
	return Cast<UFormCharacterComponent>(GetOwner()->FindComponentByClass(UFormCharacterComponent::StaticClass()));
}

void USfObject::ErrorIfNoAuthority() const
{
	const AActor* Owner = GetOwner();
	checkf(Owner != nullptr, TEXT("Invalid owner."));
	checkf(Owner->HasAuthority(), TEXT("Called without authority."));
}

bool USfObject::HasAuthority() const
{
	return GetOwner() && GetOwner()->HasAuthority();
}
