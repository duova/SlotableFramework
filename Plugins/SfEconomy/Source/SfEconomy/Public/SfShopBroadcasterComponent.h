// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SfShopBroadcasterComponent.generated.h"

class UFormCoreComponent;
class USfCurrencyComponent;
class USfShopAccessorComponent;
class UInventory;
struct FCurrencyValuePair;
class USlotable;

//Predicate that can be used to verify that a slotable matches certain conditions. The event
//Check must be implemented to be used.
UCLASS(Blueprintable)
class SFECONOMY_API USlotableCondition : public UObject
{
	GENERATED_BODY()

public:

	USlotableCondition();
	
	UFUNCTION(BlueprintImplementableEvent)
	void Check(const USlotable* InSlotable, bool& bOutResult);
};

//Conditions for a slotable including the class.
USTRUCT(BlueprintType)
struct SFECONOMY_API FSlotableClassAndConditions
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<USlotable> SlotableClass;

	UPROPERTY(BlueprintReadWrite)
	TArray<TSubclassOf<USlotableCondition>> Conditions;

	FSlotableClassAndConditions();

	bool operator==(const FSlotableClassAndConditions& Other) const;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

//A data asset that represents a shop offer than can be put on different shops.
//Note that the offers are what is being offered to the buyer, while the requests are what
//is wanted from the buyer.
UCLASS(BlueprintType)
class SFECONOMY_API UShopOffer : public UDataAsset
{
	GENERATED_BODY()

public:
	//Describe a class it must be and conditions it must meet to be valid for the offer.
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<TSubclassOf<USlotable>> SlotableOffers;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<FCurrencyValuePair> CurrencyOffers;

	//Describe a class it must be and conditions it must meet to be valid for the offer.
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<FSlotableClassAndConditions> SlotableRequests;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<FCurrencyValuePair> CurrencyRequests;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	bool bIsInfinite;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TSubclassOf<UInventory> DefaultInventoryClass;

	UShopOffer();
};

//Number of each offer available. Amount is neglected if bIsInfinite is true for an offer.
USTRUCT(BlueprintType)
struct SFECONOMY_API FShopOfferWithAmount
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	const UShopOffer* ShopOffer;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (ClampMin = 1))
	int32 Amount = 1;

	FShopOfferWithAmount();

	FShopOfferWithAmount(const UShopOffer* InShopOffer, int32 InAmount);

	bool operator==(const FShopOfferWithAmount& Other) const;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnPurchase, USfShopAccessorComponent*, Accessor, const UShopOffer*, ShopOffer, const int32, Amount, TArray<USlotable*>, ObtainedSlotables);

//A component that allows an actor to act as a shop and serve ShopOffers.
//This class must be extended and VerifyAccessorCanAccessShop implemented for it to know under what conditions a
//ShopAccessorComponent can access the shop.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class SFECONOMY_API USfShopBroadcasterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USfShopBroadcasterComponent();

protected:
	virtual void BeginPlay() override;

public:
	//Called on the server when a purchase is made from this shop.
	FOnPurchase Server_OnPurchase;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintImplementableEvent)
	void VerifyAccessorCanAccessShop(const USfShopAccessorComponent* InAccessor, bool& bCanAccess) const;

	UFUNCTION(BlueprintCallable)
	const TArray<FShopOfferWithAmount>& GetShopOffers() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Server_AddShopOffer(const UShopOffer* InShopOffer, const int32 InAmount = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_RemoveShopOffer(const UShopOffer* InShopOffer);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_SetShopOfferAmount(const UShopOffer* InShopOffer, const int32 InAmount);
	
	//Leaving TargetInventory as null will put the slotables into the first inventory that is of the default inventory class.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_Purchase(USfShopAccessorComponent* InAccessor, const UShopOffer* InShopOffer, const int32 InAmount = 1, TArray<USlotable*> OfferedSlotables = TArray<USlotable*>(), UInventory* TargetInventory = nullptr);

	static void FindMatchingSlotables(const TArray<USlotable*>& InOfferedSlotables, TArray<USlotable*>& OutSlotablesToTrade, const FSlotableClassAndConditions& InClassAndConditions, const int32 InAmount);

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bRemoveSlotsWhenTakingSlotables = false;

private:
	UPROPERTY(EditAnywhere, Replicated)
	TArray<FShopOfferWithAmount> ShopOffers;
};
