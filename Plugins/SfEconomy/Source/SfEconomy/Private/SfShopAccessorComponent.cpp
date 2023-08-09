// Fill out your copyright notice in the Description page of Project Settings.


#include "SfShopAccessorComponent.h"

#include "FormCoreComponent.h"
#include "SfCurrencyComponent.h"
#include "SfShopBroadcasterComponent.h"

USfShopAccessorComponent::USfShopAccessorComponent()
{
	if (!GetOwner()) return;
	PrimaryComponentTick.bCanEverTick = false;
	bReplicateUsingRegisteredSubObjectList = true;
	SetIsReplicatedByDefault(true);
	bAttemptedCurrencyComponentCaching = false;
	bAttemptedFormCoreCaching = false;
}

void USfShopAccessorComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void USfShopAccessorComponent::Client_Purchase(USfShopBroadcasterComponent* Shop, const UShopOffer* InShopOffer,
	const int32 InAmount, const TArray<USlotable*>& InOfferedSlotables)
{
	ClientPurchase(Shop, InShopOffer, InAmount, InOfferedSlotables);
}

void USfShopAccessorComponent::InternalClientRpcForPurchaseCallback_Implementation(const EPurchaseResponse InResponse)
{
	Client_PurchaseCallback(InResponse);
}

USfCurrencyComponent* USfShopAccessorComponent::GetCurrencyComponent()
{
	if (bAttemptedCurrencyComponentCaching) return CachedCurrencyComponent;
	CachedCurrencyComponent = Cast<USfCurrencyComponent>(GetOwner()->FindComponentByClass(USfCurrencyComponent::StaticClass()));
	bAttemptedCurrencyComponentCaching = true;
	return CachedCurrencyComponent;
}

UFormCoreComponent* USfShopAccessorComponent::GetFormCore()
{
	if (bAttemptedFormCoreCaching) return CachedFormCore;
	CachedFormCore = Cast<UFormCoreComponent>(GetOwner()->FindComponentByClass(UFormCoreComponent::StaticClass()));
	bAttemptedFormCoreCaching = true;
	return CachedFormCore;
}

void USfShopAccessorComponent::ClientPurchase_Implementation(USfShopBroadcasterComponent* Shop, const UShopOffer* InShopOffer,
                                                              const int32 InAmount, const TArray<USlotable*>& InOfferedSlotables)
{
	Shop->ServerPurchase(this, InShopOffer, InOfferedSlotables, InAmount);
}