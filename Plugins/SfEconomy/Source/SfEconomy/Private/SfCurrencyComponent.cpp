// Fill out your copyright notice in the Description page of Project Settings.


#include "SfCurrencyComponent.h"

#include "SfObject.h"
#include "Net/UnrealNetwork.h"

FCurrencyValuePair::FCurrencyValuePair(): Value(0)
{
}

bool FCurrencyValuePair::operator==(const FCurrencyValuePair& Other) const
{
	if (Currency == Other.Currency && Value == Other.Value) return true;
	return false;
}

bool FCurrencyValuePair::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	Ar << Currency;
	Ar << Value;
	return bOutSuccess;
}

USfCurrencyComponent::USfCurrencyComponent()
{
	if (!GetOwner()) return;
	PrimaryComponentTick.bCanEverTick = false;
	bReplicateUsingRegisteredSubObjectList = true;
	SetIsReplicatedByDefault(true);
}

void USfCurrencyComponent::BeginPlay()
{
	Super::BeginPlay();
	if (USfObject::TArrayCheckDuplicate(HeldCurrencyValues, [](const FCurrencyValuePair& A, const FCurrencyValuePair& B){return A.Currency == B.Currency;}))
	{
		UE_LOG(LogTemp, Error, TEXT("Currencies duplicated."));
	}
}

void USfCurrencyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(USfCurrencyComponent, HeldCurrencyValues, DefaultParams);
}

bool USfCurrencyComponent::Server_HasCurrency(const FGameplayTag Currency) const
{
	return HeldCurrencyValues.FindByPredicate([Currency](const FCurrencyValuePair Pair){ return Pair.Currency == Currency; }) == nullptr;
}

int32 USfCurrencyComponent::Server_GetCurrencyValue(const FGameplayTag Currency) const
{
	for (const FCurrencyValuePair& Pair : HeldCurrencyValues)
	{
		if (Pair.Currency != Currency) continue;
		return Pair.Value;
	}
	return 0;
}

bool USfCurrencyComponent::Server_AddCurrency(const FGameplayTag Currency, const int32 Value)
{
	if (Value < 0) return false;
	for (FCurrencyValuePair& Pair : HeldCurrencyValues)
	{
		if (Pair.Currency != Currency) continue;
		const uint32 Result = Pair.Value + Value;
		if (Result > USfObject::Int32MaxValue) return false;
		Pair.Value = Result;
		MARK_PROPERTY_DIRTY_FROM_NAME(USfCurrencyComponent, HeldCurrencyValues, this);
		return true;
	}
	return false;
}

bool USfCurrencyComponent::Server_DeductCurrency(const FGameplayTag Currency, const int32 Value)
{
	if (Value < 0) return false;
	for (FCurrencyValuePair& Pair : HeldCurrencyValues)
	{
		if (Pair.Currency != Currency) continue;
		const int32 Result = Pair.Value - Value;
		Pair.Value = FMath::Clamp(Result, 0, Pair.Value);
		MARK_PROPERTY_DIRTY_FROM_NAME(USfCurrencyComponent, HeldCurrencyValues, this);
		return true;
	}
	return false;
}

bool USfCurrencyComponent::Server_SetCurrency(const FGameplayTag Currency, const int32 Value)
{
	if (Value < 0) return false;
	for (FCurrencyValuePair& Pair : HeldCurrencyValues)
	{
		if (Pair.Currency != Currency) continue;
		Pair.Value = Value;
		MARK_PROPERTY_DIRTY_FROM_NAME(USfCurrencyComponent, HeldCurrencyValues, this);
		return true;
	}
	return false;
}

