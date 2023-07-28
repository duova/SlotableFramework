// Fill out your copyright notice in the Description page of Project Settings.


#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "SfTradeComponent.generated.h"

/*
class UInventory;
class UFormCoreComponent;
class USfTradeComponent;
class USlotable;

USTRUCT(BlueprintType)
struct SFECONOMY_API FTradeOffer
{
	GENERATED_BODY()

	float TimeoutTimestamp;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TArray<USlotable*> SlotableOffers;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TArray<FCurrencyValuePair> CurrencyOffers;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TArray<USlotable*> SlotableRequests;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TArray<FCurrencyValuePair> CurrencyRequests;

	FTradeOffer();

	//Abstract slotables describe a class it must be and conditions it must meet to be valid for the offer.
	explicit FTradeOffer(const TArray<USlotable*> InSlotableOffers = TArray<USlotable*>(),
	                     const TArray<FCurrencyValuePair> InCurrencyOffers = TArray<FCurrencyValuePair>(),
	                     const TArray<USlotable*> InSlotableRequests = TArray<USlotable*>(),
	                     const TArray<FCurrencyValuePair> InCurrencyRequests = TArray<FCurrencyValuePair>());

	//Doesn't check if the timeout is identical.
	bool operator==(const FTradeOffer& Other) const;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
	
	bool operator!=(const FTradeOffer& Other) const;
};

//Trade offer with the source of the trade offer.
USTRUCT(BlueprintType)
struct SFECONOMY_API FSignedTradeOffer
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	USfTradeComponent* Origin;

	UPROPERTY(BlueprintReadWrite)
	FTradeOffer TradeOffer;

	FSignedTradeOffer();

	FSignedTradeOffer(USfTradeComponent* InOrigin, const FTradeOffer& InTradeOffer);

	bool operator==(const FSignedTradeOffer& Other) const;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

//Implementation of what to do with slotables received from trades.
UCLASS(Blueprintable)
class SFECONOMY_API UTradedSlotableInventoryFinder : public UObject
{
	GENERATED_BODY()

public:
	UTradedSlotableInventoryFinder();

	UFUNCTION(BlueprintImplementableEvent)
	void FindInventory(const USlotable* InSlotableBeforeTrade, const UFormCoreComponent* InTargetForm, UInventory* OutInventoryToPlaceIn);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnIncomingOfferAccepted, USfTradeComponent*, Origin, const FTradeOffer&, TradeOffer, TArray<USlotable*>, SlotablesRecieved, TArray<USlotable*>, SlotablesSent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnOutgoingOfferAccepted, USfTradeComponent*, Recipient, const FTradeOffer&, TradeOffer, TArray<USlotable*>, SlotablesRecieved, TArray<USlotable*>, SlotablesSent);

// A component that allows an actor to perform trades with other actors with the same component using currencies or
// slotables.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class SFECONOMY_API USfTradeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USfTradeComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

public:
	FOnOutgoingOfferAccepted OnOutgoingOfferAccepted;

	FOnIncomingOfferAccepted OnIncomingOfferAccepted;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float DefaultOfferTimeout;

	//Returns false if trade offer isn't valid due to missing resources.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_SendTradeOffer(USfTradeComponent* Recipient, FTradeOffer TradeOffer, float Timeout);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Server_AcceptTradeOffer(FSignedTradeOffer TradeOffer);

	//Note that the timeout on the trade offer will get overriden by the server
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Client_SendTradeOffer(USfTradeComponent* Recipient, FTradeOffer TradeOffer);
	
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Client_AcceptTradeOffer(FSignedTradeOffer TradeOffer);

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void OfferAcceptingAck(const bool bOfferAccepted);

	UFUNCTION(BlueprintImplementableEvent)
	void Client_OnOfferAcceptingAck(bool bOfferAccepted);

	UFUNCTION(BlueprintImplementableEvent)
	void Client_OnRecieveNewTradeOffer(bool bOfferAccepted);

	UFUNCTION(BlueprintPure)
	const TArray<FSignedTradeOffer>& GetPendingTradeOffers();

	UFUNCTION()
	void OnRep_PendingTradeOffers();

	UFUNCTION(BlueprintPure)
	float GetTimeUntilTradeOfferTimeout(const FTradeOffer& TradeOffer);
	
private:

	UPROPERTY(Replicated)
	TArray<FSignedTradeOffer> PendingTradeOffers;

	TSet<FSignedTradeOffer&> ClientHandledTradeOffers;
};

*/