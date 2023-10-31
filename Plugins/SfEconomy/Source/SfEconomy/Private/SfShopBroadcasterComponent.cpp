// Fill out your copyright notice in the Description page of Project Settings.


#include "SfShopBroadcasterComponent.h"

#include "Inventory.h"
#include "SfCurrencyComponent.h"
#include "SfObject.h"
#include "SfShopAccessorComponent.h"
#include "SfUtility.h"
#include "Slotable.h"
#include "Net/UnrealNetwork.h"

USfShopBroadcasterComponent::USfShopBroadcasterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bReplicateUsingRegisteredSubObjectList = true;
	SetIsReplicatedByDefault(true);
}

void USfShopBroadcasterComponent::BeginPlay()
{
	Super::BeginPlay();
	if (TArrayCheckDuplicate(ShopOffers, [](const FShopOfferWithAmount& A, const FShopOfferWithAmount& B)
	{
		return A.ShopOffer == B.ShopOffer;
	}))
	{
		UE_LOG(LogSfEconomy, Error, TEXT("Shop offers duplicated on SfShopBroadcasterComponent class %s."), *GetClass()->GetName());
	}
}

void USfShopBroadcasterComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(USfShopBroadcasterComponent, ShopOffers, DefaultParams);
}

const TArray<FShopOfferWithAmount>& USfShopBroadcasterComponent::GetShopOffers() const
{
	return ShopOffers;
}

void USfShopBroadcasterComponent::Server_AddShopOffer(const UShopOffer* InShopOffer, const int32 InAmount)
{
	ServerAddShopOffer(InShopOffer, InAmount);
}

void USfShopBroadcasterComponent::ServerAddShopOffer(const UShopOffer* InShopOffer, const int32 InAmount)
{
	if (InAmount < 0) return;
	for (FShopOfferWithAmount& ShopOffer : ShopOffers)
	{
		if (ShopOffer.ShopOffer == InShopOffer)
		{
			uint64 NewAmount = ShopOffer.Amount + InAmount;
			if (NewAmount > USfObject::Int32MaxValue) NewAmount = USfObject::Int32MaxValue;
			ShopOffer.Amount = NewAmount;
			return;
		}
	}
	ShopOffers.Add(FShopOfferWithAmount(InShopOffer, InAmount));
}

bool USfShopBroadcasterComponent::Server_RemoveShopOffer(const UShopOffer* InShopOffer)
{
	return ServerRemoveShopOffer(InShopOffer);
}

bool USfShopBroadcasterComponent::ServerRemoveShopOffer(const UShopOffer* InShopOffer)
{
	if (!InShopOffer) return false;
	for (uint16 i = 0; i < ShopOffers.Num(); i++)
	{
		if (ShopOffers[i].ShopOffer == InShopOffer)
		{
			ShopOffers.RemoveAt(i);
			return true;
		}
	}
	return false;
}

bool USfShopBroadcasterComponent::Server_SetShopOfferAmount(const UShopOffer* InShopOffer, const int32 InAmount)
{
	return ServerSetShopOfferAmount(InShopOffer, InAmount);
}

bool USfShopBroadcasterComponent::ServerSetShopOfferAmount(const UShopOffer* InShopOffer, const int32 InAmount)
{
	if (!InShopOffer) return false;
	if (InAmount < 0) return false;
	for (uint16 i = 0; i < ShopOffers.Num(); i++)
	{
		if (ShopOffers[i].ShopOffer == InShopOffer)
		{
			if (InAmount == 0)
			{
				ShopOffers.RemoveAt(i);
				return true;
			}
			ShopOffers[i].Amount = InAmount;
			return true;
		}
	}
	return false;
}

bool USfShopBroadcasterComponent::Server_Purchase(USfShopAccessorComponent* InAccessor, const UShopOffer* InShopOffer,
                                                  const TArray<USlotable*>& InOfferedSlotables, const int32 InAmount,
                                                  UInventory* TargetInventory)
{
	return ServerPurchase(InAccessor, InShopOffer, InOfferedSlotables, InAmount, TargetInventory);
}

void USfShopBroadcasterComponent::FindMatchingSlotables(const TArray<USlotable*>& InOfferedSlotables,
                                                        TArray<USlotable*>& OutSlotablesToTrade,
                                                        const FSlotableClassAndConditions& InClassAndConditions,
                                                        int32 InAmount)
{
	for (USlotable* Slotable : InOfferedSlotables)
	{
		if (Slotable->GetClass() != InClassAndConditions.SlotableClass.Get()) continue;
		bool bSlotableMeetsConditions = true;
		for (const TSubclassOf<USlotableCondition>& Condition : InClassAndConditions.Conditions)
		{
			if (!Condition.Get()) continue;
			bool bResult = false;
			Condition.GetDefaultObject()->Check(Slotable, bResult);
			bSlotableMeetsConditions &= bResult;
			if (!bSlotableMeetsConditions) break;
		}
		if (bSlotableMeetsConditions)
		{
			OutSlotablesToTrade.Add(Slotable);
			InAmount--;
			if (InAmount <= 0) return;
		}
	}
}

bool USfShopBroadcasterComponent::ServerPurchase(USfShopAccessorComponent* InAccessor, const UShopOffer* InShopOffer,
                                                 const TArray<USlotable*>& InOfferedSlotables, const int32 InAmount,
                                                 UInventory* TargetInventory)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogSfEconomy, Error, TEXT("ServerPurchase called without authority on USfShopBroadcasterComponent class %s."), *GetClass()->GetName());
		return false;
	}

	if (!TargetInventory)
	{
		for (UInventory* Inventory : InAccessor->GetFormCore()->GetInventories())
		{
			if (Inventory->GetClass() == InShopOffer->DefaultInventoryClass) TargetInventory = Inventory;
		}
		if (!TargetInventory)
		{
			UE_LOG(LogSfEconomy, Error, TEXT("ServerPurchase on USfShopBroadcasterComponent class %s could not locate a suitable inventory."), *GetClass()->GetName());
			InAccessor->InternalClientRpcForPurchaseCallback(EPurchaseResponse::ErrorNoSuitableInventoryToReceiveSlotables);
			return false;
		}
	}

	if (!InShopOffer) return false;
	//Run the user implemented check to ensure that the accessor meets the conditions required to access the shop.
	bool bCanAccessShop = false;
	VerifyAccessorCanAccessShop(InAccessor, bCanAccessShop);
	if (!bCanAccessShop)
	{
		InAccessor->InternalClientRpcForPurchaseCallback(EPurchaseResponse::CannotAccessShop);
		return false;
	}

	//Verify that all inventories and all slotables come from the actor.
	if (!InAccessor)
	{
		UE_LOG(LogSfEconomy, Error, TEXT("ServerPurchase on USfShopBroadcasterComponent class %s called with a null accessor."), *GetClass()->GetName());
		InAccessor->InternalClientRpcForPurchaseCallback(EPurchaseResponse::ErrorShopAccessorIsNull);
		return false;
	}
	const AActor* Actor = InAccessor->GetOwner();
	if (!Actor)
	{
		UE_LOG(LogSfEconomy, Error, TEXT("ServerPurchase on USfShopBroadcasterComponent class %s could not get accessor actor."), *GetClass()->GetName());
		InAccessor->InternalClientRpcForPurchaseCallback(EPurchaseResponse::ErrorAccessorGivenHasNoActor);
		return false;
	}
	if (TargetInventory)
	{
		if (TargetInventory->OwningFormCore->GetOwner() != Actor)
		{
			UE_LOG(LogSfEconomy, Error, TEXT("ServerPurchase on USfShopBroadcasterComponent class %s target inventory is not on the same actor as the accessor."), *GetClass()->GetName());
			InAccessor->InternalClientRpcForPurchaseCallback(EPurchaseResponse::ErrorVariablesGivenBelongToDifferentActors);
			return false;
		}
	}
	for (const USlotable* Slotable : InOfferedSlotables)
	{
		if (Slotable->OwningInventory->OwningFormCore->GetOwner() != Actor)
		{
			UE_LOG(LogSfEconomy, Error, TEXT("ServerPurchase on USfShopBroadcasterComponent class %s has an offered slotable that is not on the same actor as the accessor."), *GetClass()->GetName());
			InAccessor->InternalClientRpcForPurchaseCallback(EPurchaseResponse::ErrorVariablesGivenBelongToDifferentActors);
			return false;
		}
	}

	//Cache the FShopOfferWithAmount for later operations.
	FShopOfferWithAmount* SelectedOfferWithAmount = ShopOffers.FindByPredicate(
		[InShopOffer](const FShopOfferWithAmount Offer)
		{
			if (Offer.ShopOffer == InShopOffer) return true;
			return false;
		});

	//Check that shop is stocked.
	if (!InShopOffer->bIsInfinite)
	{
		if (SelectedOfferWithAmount->Amount < InAmount)
		{
			InAccessor->InternalClientRpcForPurchaseCallback(EPurchaseResponse::OfferOutOfStock);
			return false;
		}
	}

	//Check currency.
	for (const FCurrencyValuePair& Pair : InShopOffer->CurrencyRequests)
	{
		const int32 HeldValue = InAccessor->GetCurrencyComponent()->Server_GetCurrencyValue(Pair.Currency);
		if (Pair.Value * InAmount > HeldValue)
		{
			InAccessor->InternalClientRpcForPurchaseCallback(EPurchaseResponse::NotEnoughCurrency);
			return false;
		}
	}

	//Cache slotables that would be traded to check that buyer has necessary resources.
	TArray<USlotable*> SlotablesToTrade;
	for (const FSlotableClassAndConditions& ClassAndConditions : InShopOffer->SlotableRequests)
	{
		FindMatchingSlotables(InOfferedSlotables, SlotablesToTrade, ClassAndConditions, InAmount);
	}
	if (SlotablesToTrade.Num() != InShopOffer->SlotableRequests.Num() * InAmount)
	{
		InAccessor->InternalClientRpcForPurchaseCallback(EPurchaseResponse::LackingSlotablesRequired);
		return false;
	}

	//Check that there is enough inventory space for the purchase.
	if (TargetInventory->GetRemainingCapacity() < InShopOffer->SlotableOffers.Num())
	{
		InAccessor->InternalClientRpcForPurchaseCallback(EPurchaseResponse::LackingInventorySpace);
		return false;
	}

	//Take resources.
	for (const FCurrencyValuePair& Pair : InShopOffer->CurrencyRequests)
	{
		InAccessor->GetCurrencyComponent()->Server_DeductCurrency(Pair.Currency, Pair.Value * InAmount);
	}
	for (USlotable* Slotable : SlotablesToTrade)
	{
		Slotable->OwningInventory->Server_RemoveSlotable(Slotable, bRemoveSlotsWhenTakingSlotables);
	}

	//Give resources.
	for (const FCurrencyValuePair& Pair : InShopOffer->CurrencyRequests)
	{
		InAccessor->GetCurrencyComponent()->Server_AddCurrency(Pair.Currency, Pair.Value * InAmount);
	}
	TArray<USlotable*> AddedSlotables;
	for (int32 i = 0; i < InAmount; i++)
	{
		for (const TSubclassOf<USlotable>& SlotableClass : InShopOffer->SlotableOffers)
		{
			AddedSlotables.Add(TargetInventory->Server_AddSlotable(SlotableClass, nullptr));
		}
	}

	//Deduct amount.
	if (!InShopOffer->bIsInfinite)
	{
		SelectedOfferWithAmount->Amount -= InAmount;
		if (SelectedOfferWithAmount->Amount <= 0) Server_RemoveShopOffer(InShopOffer);
	}

	Server_OnPurchase.Broadcast(InAccessor, InShopOffer, InAmount, AddedSlotables);
	InAccessor->InternalClientRpcForPurchaseCallback(EPurchaseResponse::Success);
	
	return true;
}

USlotableCondition::USlotableCondition()
{
}

FSlotableClassAndConditions::FSlotableClassAndConditions()
{
}

bool FSlotableClassAndConditions::operator==(const FSlotableClassAndConditions& Other) const
{
	if (SlotableClass != Other.SlotableClass) return false;
	if (Conditions != Other.Conditions) return false;
	return true;
}

bool FSlotableClassAndConditions::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	//Direct class serializations are relatively acceptable here since abstract slotables are usually used in tandem
	//with trade offer presets, which means they don't get serialized anyway. Even if it is used the frequency is low,
	//and the cost of indexing all the classes might be higher if this form of serialization isn't used.
	bOutSuccess = true;
	Ar << SlotableClass;
	Ar << Conditions;
	return bOutSuccess;
}

UShopOffer::UShopOffer()
{
}

FShopOfferWithAmount::FShopOfferWithAmount(): ShopOffer(nullptr)
{
}

FShopOfferWithAmount::FShopOfferWithAmount(const UShopOffer* InShopOffer, const int32 InAmount): ShopOffer(nullptr)
{
	ShopOffer = InShopOffer;
	Amount = InAmount;
}

bool FShopOfferWithAmount::operator==(const FShopOfferWithAmount& Other) const
{
	return ShopOffer == Other.ShopOffer && Amount == Other.Amount;
}

bool FShopOfferWithAmount::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	UObject* ShopOfferObject = const_cast<UShopOffer*>(ShopOffer);
	Map->SerializeObject(Ar, ShopOffer->GetClass(), ShopOfferObject);
	ShopOffer = static_cast<UShopOffer*>(ShopOfferObject);
	Ar << Amount;
	return bOutSuccess;
}
