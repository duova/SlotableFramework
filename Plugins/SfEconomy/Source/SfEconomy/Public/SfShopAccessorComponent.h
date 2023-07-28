// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SfShopAccessorComponent.generated.h"


class UFormCoreComponent;
class USfCurrencyComponent;
class USfShopBroadcasterComponent;
class USlotable;
class UShopOffer;

//Ones prefixed with Error are considered issues with framework implementation and is unintended behaviour.
UENUM(BlueprintType)
enum class EPurchaseResponse
{
	Success,
	OfferOutOfStock,
	NotEnoughCurrency,
	LackingSlotablesRequired,
	LackingInventorySpace,
	CannotAccessShop,
	ErrorNoSuitableInventoryToReceiveSlotables,
	ErrorShopAccessorIsNull,
	ErrorAccessorGivenHasNoActor,
	ErrorVariablesGivenBelongToDifferentActors,
};

//A component that is used to access shops. The event Client_PurchaseCallback must be implemented to be used.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class SFECONOMY_API USfShopAccessorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USfShopAccessorComponent();

protected:
	virtual void BeginPlay() override;

public:
	//Call to attempt to make a purchase at a certain shop. PurchaseCallback will be called as a response with a enum
	//that represents the approximate result.
	UFUNCTION(BlueprintCallable, Server)
	void Client_Purchase(USfShopBroadcasterComponent* Shop, const UShopOffer* InShopOffer, const int32 InAmount, TArray<USlotable*> OfferedSlotables = TArray<USlotable*>());

	UFUNCTION(Client)
	void InternalClientRpcForPurchaseCallback(const EPurchaseResponse Response);

	//Called as a response whenever a purchase is attempted by the client.
	UFUNCTION(BlueprintImplementableEvent)
	void Client_PurchaseCallback(const EPurchaseResponse Response);

	USfCurrencyComponent* GetCurrencyComponent();

	UFormCoreComponent* GetFormCore();

private:
	UPROPERTY()
	USfCurrencyComponent* CachedCurrencyComponent;

	uint8 bAttemptedCurrencyComponentCaching:1;

	UPROPERTY()
	UFormCoreComponent* CachedFormCore;

	uint8 bAttemptedFormCoreCaching:1;
};
