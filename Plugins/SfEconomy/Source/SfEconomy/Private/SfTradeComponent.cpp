// Fill out your copyright notice in the Description page of Project Settings.
/*

#include "SfTradeComponent.h"

#include "Slotable.h"
#include "Net/UnrealNetwork.h"

FTradeOffer::FTradeOffer(): TimeoutTimestamp(0)
{
}

FTradeOffer::FTradeOffer(const TArray<USlotable*> InSpecificSlotableOffers,
                         const TArray<FSlotableClassAndConditions> InAbstractSlotableOffers,
                         const TArray<FCurrencyValuePair> InCurrencyOffers, const TArray<USlotable*> InSpecificSlotableRequests,
                         const TArray<FSlotableClassAndConditions> InAbstractSlotableRequests,
                         const TArray<FCurrencyValuePair> InCurrencyRequests)
{
	SpecificSlotableOffers = InSpecificSlotableOffers;
	AbstractSlotableOffers = InAbstractSlotableOffers;
	CurrencyOffers = InCurrencyOffers;
	SpecificSlotableRequests = InSpecificSlotableRequests;
	AbstractSlotableRequests = InAbstractSlotableRequests;
	CurrencyRequests = InCurrencyRequests;
	TimeoutTimestamp = 0;
}

bool FTradeOffer::operator==(const FTradeOffer& Other) const
{
	if (!USfObject::TArrayCompareOrderless(SpecificSlotableOffers, Other.SpecificSlotableOffers)) return false;
	if (!USfObject::TArrayCompareOrderless(AbstractSlotableOffers, Other.AbstractSlotableOffers)) return false;
	if (CurrencyOffers != Other.CurrencyOffers) return false;
	if (!USfObject::TArrayCompareOrderless(SpecificSlotableRequests, Other.SpecificSlotableRequests)) return false;
	if (!USfObject::TArrayCompareOrderless(AbstractSlotableRequests, Other.AbstractSlotableRequests)) return false;
	if (CurrencyRequests != Other.CurrencyRequests) return false;
	return true;
}

bool FTradeOffer::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	Ar << SpecificSlotableOffers;
	Ar << AbstractSlotableOffers;
	Ar << CurrencyOffers;
	Ar << SpecificSlotableRequests;
	Ar << AbstractSlotableRequests;
	Ar << CurrencyRequests;
	Ar << TimeoutTimestamp;
	return bOutSuccess;
}

bool FTradeOffer::operator!=(const FTradeOffer& Other) const
{
	return !(*this == Other);
}

FSignedTradeOffer::FSignedTradeOffer(): Origin(nullptr)
{
}

FSignedTradeOffer::FSignedTradeOffer(USfTradeComponent* InOrigin, const FTradeOffer& InTradeOffer)
{
	Origin = InOrigin;
	TradeOffer = InTradeOffer;
}

bool FSignedTradeOffer::operator==(const FSignedTradeOffer& Other) const
{
	if (Origin != Other.Origin) return false;
	if (TradeOffer != Other.TradeOffer) return false;
	return true;
}

bool FSignedTradeOffer::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	Ar << Origin;
	TradeOffer.NetSerialize(Ar, Map, bOutSuccess);
	return bOutSuccess;
}

USfTradeComponent::USfTradeComponent()
{
	if (!GetOwner()) return;
	PrimaryComponentTick.bCanEverTick = false;
	bReplicateUsingRegisteredSubObjectList = true;
	SetIsReplicated(true);
}

void USfTradeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	FDoRepLifetimeParams AutoTradeGroupParams;
	AutoTradeGroupParams.bIsPushBased = true;
	//Only send auto trade groups at the start because they shouldn't change.
	AutoTradeGroupParams.Condition = COND_InitialOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(USfTradeComponent, PendingTradeOffers, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(USfTradeComponent, AutoTradeGroups, AutoTradeGroupParams);
}

void USfTradeComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool USfTradeComponent::Server_SendTradeOffer(USfTradeComponent* Recipient, FTradeOffer TradeOffer, float Timeout)
{
	
}

bool USfTradeComponent::Server_SendTradeOfferPreset(USfTradeComponent* Recipient, UTradeOfferPreset* TradeOfferPreset,
	float Timeout)
{
}

void USfTradeComponent::Server_AcceptTradeOffer(FSignedTradeOffer TradeOffer)
{
}

const TArray<UAutoTradeGroup*>& USfTradeComponent::GetAutoTradeGroups()
{
}

void USfTradeComponent::Client_SendTradeOfferPreset_Implementation(USfTradeComponent* Recipient,
	UTradeOfferPreset* TradeOfferPreset)
{
}

const TArray<FSignedTradeOffer>& USfTradeComponent::GetPendingTradeOffers()
{
}

void USfTradeComponent::OnRep_PendingTradeOffers()
{
	//Compare to handled trade offers to know what new ones to call receive on.
}

float USfTradeComponent::GetTimeUntilTradeOfferTimeout(const FTradeOffer& TradeOffer)
{
}

void USfTradeComponent::OfferAcceptingAck_Implementation(const bool bOfferAccepted)
{
	Client_OnOfferAcceptingAck(bOfferAccepted);
}

void USfTradeComponent::Client_AcceptTradeOffer_Implementation(FSignedTradeOffer TradeOffer)
{
}

void USfTradeComponent::Client_SendTradeOffer_Implementation(USfTradeComponent* Recipient, FTradeOffer TradeOffer)
{
	//Override client sent timeout.
}

*/