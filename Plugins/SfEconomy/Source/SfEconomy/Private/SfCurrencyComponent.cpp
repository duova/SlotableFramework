// Fill out your copyright notice in the Description page of Project Settings.


#include "SfCurrencyComponent.h"

#include "SfObject.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogSfEconomy);

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
	Currency.NetSerialize_Packed(Ar, Map, bOutSuccess);
	Ar << Value;
	return bOutSuccess;
}

USfCurrencyComponent::USfCurrencyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bReplicateUsingRegisteredSubObjectList = true;
	SetIsReplicatedByDefault(true);
}

void USfCurrencyComponent::BeginPlay()
{
	Super::BeginPlay();
	if (TArrayCheckDuplicate(HeldCurrencyValues, [](const FCurrencyValuePair& A, const FCurrencyValuePair& B){return A.Currency == B.Currency;}))
	{
		UE_LOG(LogSfEconomy, Error, TEXT("Currencies tags duplicated on SfCurrencyComponent class %s. Only one will be used."), *GetClass()->GetName());
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

bool USfCurrencyComponent::Server_HasCurrency(const FGameplayTag InCurrency) const
{
	return HeldCurrencyValues.FindByPredicate([InCurrency](const FCurrencyValuePair Pair){ return Pair.Currency == InCurrency; }) == nullptr;
}

int32 USfCurrencyComponent::Server_GetCurrencyValue(const FGameplayTag InCurrency) const
{
	for (const FCurrencyValuePair& Pair : HeldCurrencyValues)
	{
		if (Pair.Currency != InCurrency) continue;
		return Pair.Value;
	}
	return 0;
}

bool USfCurrencyComponent::Server_AddCurrency(const FGameplayTag InCurrency, const int32 InValue)
{
	if (InValue < 0) return false;
	for (FCurrencyValuePair& Pair : HeldCurrencyValues)
	{
		if (Pair.Currency != InCurrency) continue;
		const uint32 Result = Pair.Value + InValue;
		if (Result > USfObject::Int32MaxValue) return false;
		Pair.Value = Result;
		MARK_PROPERTY_DIRTY_FROM_NAME(USfCurrencyComponent, HeldCurrencyValues, this);
		return true;
	}
	return false;
}

bool USfCurrencyComponent::Server_DeductCurrency(const FGameplayTag InCurrency, const int32 InValue)
{
	if (InValue < 0) return false;
	for (FCurrencyValuePair& Pair : HeldCurrencyValues)
	{
		if (Pair.Currency != InCurrency) continue;
		const int32 Result = Pair.Value - InValue;
		Pair.Value = FMath::Clamp(Result, 0, Pair.Value);
		MARK_PROPERTY_DIRTY_FROM_NAME(USfCurrencyComponent, HeldCurrencyValues, this);
		return true;
	}
	return false;
}

bool USfCurrencyComponent::Server_SetCurrency(const FGameplayTag InCurrency, const int32 InValue)
{
	if (InValue < 0) return false;
	for (FCurrencyValuePair& Pair : HeldCurrencyValues)
	{
		if (Pair.Currency != InCurrency) continue;
		Pair.Value = InValue;
		MARK_PROPERTY_DIRTY_FROM_NAME(USfCurrencyComponent, HeldCurrencyValues, this);
		return true;
	}
	return false;
}

