// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "Card.h"
#include "SfObject.h"
#include "Inventory.generated.h"

struct FCard;
class USlotable;
class UConstituent;
class UCardObject;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAddSlotable, USlotable*, Slotable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRemoveSlotable, USlotable*, Slotable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAddCard, FCard&, Card, bool, bIsPredictableContext);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRemoveCard, FCard&, Card, bool, bIsPredictableContext);

/**
 * Inventories of slotables.
 * These can be dynamic or static, in terms of how many slotables they can contain.
 * They can also be preset with slotables, which can be locked.
 */
UCLASS(Blueprintable)
class SFCORE_API UInventory : public USfObject
{
	GENERATED_BODY()

	friend class UFormCharacterComponent;
	friend struct FBufferedInput;

public:
	UInventory();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void AuthorityTick(float DeltaTime);
	
	UFUNCTION(BlueprintPure)
	const TArray<USlotable*>& GetSlotables() const;

	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere, Category = "Inventory")
	UFormCoreComponent* OwningFormCore;

	UFUNCTION(BlueprintPure)
	TArray<USlotable*> GetSlotablesOfClass(const TSubclassOf<USlotable>& InSlotableClass) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	USlotable* Server_AddSlotable(const TSubclassOf<USlotable>& InSlotableClass, UConstituent* Origin);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_RemoveSlotable(USlotable* Slotable, const bool bInRemoveSlot);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Server_RemoveSlotableByIndex(const int32 InIndex, const bool bInRemoveSlot);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	USlotable* Server_SetSlotable(const TSubclassOf<USlotable>& InSlotableClass, const int32 InIndex, const bool bInSlotMustBeNullOrEmpty, UConstituent* Origin);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	USlotable* Server_InsertSlotable(const TSubclassOf<USlotable>& InSlotableClass, const int32 InIndex, UConstituent* Origin);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_SwapSlotables(USlotable* SlotableA, USlotable* SlotableB);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Server_SwapSlotablesByIndex(const int32 InIndexA, const int32 InIndexB);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Server_TradeSlotablesBetweenInventories(USlotable* SlotableA, USlotable* SlotableB);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta = (AutoCreateRefTerm = "InCustomLifetime"))
	bool Server_AddSharedCard(const TSubclassOf<UCardObject>& InCardClass, const float InCustomLifetime = 0);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_RemoveSharedCard(const TSubclassOf<UCardObject>& InCardClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta = (AutoCreateRefTerm = "InCustomLifetime"))
	bool Server_AddOwnedCard(const TSubclassOf<UCardObject>& InCardClass, const int32 InInOwnerConstituentInstanceId, const float InCustomLifetime = 0);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_RemoveOwnedCard(const TSubclassOf<UCardObject>& InCardClass, const int32 InOwnerConstituentInstanceId);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "InCustomLifetime"))
	bool Predicted_AddSharedCard(const TSubclassOf<UCardObject>& InCardClass, const float InCustomLifetime = 0);

	UFUNCTION(BlueprintCallable)
	bool Predicted_RemoveSharedCard(const TSubclassOf<UCardObject>& InCardClass);
	
	void UpdateAndRunBufferedInputs(UConstituent* Constituent) const;

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "InCustomLifetime"))
	bool Predicted_AddOwnedCard(const TSubclassOf<UCardObject>& InCardClass, const int32 InOwnerConstituentInstanceId, const float InCustomLifetime = 0);

	UFUNCTION(BlueprintCallable)
	bool Predicted_RemoveOwnedCard(const TSubclassOf<UCardObject>& InCardClass, const int32 InOwnerConstituentInstanceId);

	UFUNCTION(BlueprintPure)
	bool HasSlotableOfClass(const TSubclassOf<USlotable>& InSlotableClass);

	UFUNCTION(BlueprintPure)
	int32 SlotableCount(const TSubclassOf<USlotable>& InSlotableClass);

	UFUNCTION(BlueprintPure)
	bool HasSharedCard(const TSubclassOf<UCardObject>& InCardClass) const;

	UFUNCTION(BlueprintPure)
	bool HasOwnedCard(const TSubclassOf<UCardObject>& InCardClass, const int32 InOwnerConstituentInstanceId) const;

	//Will return 0 if could not find or infinite lifetime.
	UFUNCTION(BlueprintPure)
	float GetSharedCardLifetime(const TSubclassOf<UCardObject>& InCardClass);

	//Will return 0 if could not find or infinite lifetime.
	UFUNCTION(BlueprintPure)
	float GetOwnedCardLifetime(const TSubclassOf<UCardObject>& InCardClass, const int32 InOwnerConstituentInstanceId);

	//Only on client.
	UFUNCTION(BlueprintPure)
	const TArray<UCardObject*>& Client_GetCardObjects() const;

	UFUNCTION(BlueprintCallable)
	int32 GetRemainingCapacity() const;

	void ClientInitialize();

	void ServerInitialize();
	
	void ClientDeinitialize();

	void ServerDeinitialize();

	UFUNCTION(BlueprintImplementableEvent)
	void Client_Initialize();

	UFUNCTION(BlueprintImplementableEvent)
	void Server_Initialize();

	UFUNCTION(BlueprintImplementableEvent)
	void Client_Deinitialize();

	UFUNCTION(BlueprintImplementableEvent)
	void Server_Deinitialize();

	void AssignConstituentInstanceId(UConstituent* Constituent);

	void ReassignAllConstituentInstanceIds();

	void RemoveCardsOfOwner(const int32 InOwnerConstituentInstanceId);

	bool IsDynamicLength() const;

	TArray<const FCard*> GetCardsOfClass(const TSubclassOf<UCardObject> InClass) const;

	UPROPERTY(BlueprintAssignable)
	FOnAddSlotable Server_OnAddSlotable;

	UPROPERTY(BlueprintAssignable)
	FOnRemoveSlotable Server_OnRemoveSlotable;

	UPROPERTY(BlueprintAssignable)
	FOnAddCard OnAddCard;

	UPROPERTY(BlueprintAssignable)
	FOnRemoveCard OnRemoveCard;

protected:

	virtual void BeginDestroy() override;

	//These must be registered with the FormCharacterComponent first.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<UInputAction*> OrderedInputBindings;

	//Initially spawned slotables.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<TSubclassOf<USlotable>> InitialOrderedSlotableClasses;

	//Initially spawned cards with infinite lifetime.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<TSubclassOf<UCardObject>> InitialSharedCardClassesInfiniteLifetime;

	//Class that is used to fill empty inventory slots.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<USlotable> EmptySlotableClass;

	//Whether slotables can be added.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	bool bIsDynamic;

	//Whether the slotables in the inventory can be changed.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	bool bIsChangeLocked;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 1, ClampMax = 127), Category = "Inventory")
	int32 Capacity;

	//Called after a slotable is added to an inventory.
	void InitializeSlotable(USlotable* Slotable, UConstituent* Origin);
	
	//Called before a slotable is removed from an inventory.
	void DeinitializeSlotable(USlotable* Slotable);

private:

	UFUNCTION()
	void OnRep_Slotables();
	
	UPROPERTY(Replicated, VisibleAnywhere, ReplicatedUsing = OnRep_Slotables, Category = "Inventory")
	TArray<USlotable*> Slotables;

	TArray<USlotable*> ClientSubObjectListRegisteredSlotables;

	TArray<int8> OrderedInputBindingIndices;

	TBitArray<> OrderedLastInputState;

	//Does not synchronize to owner as owner should have it be predicted.
	UPROPERTY(Replicated)
	TArray<FCard> Cards;

	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TArray<UCardObject*> ClientCardObjects;
	
	USlotable* CreateUninitializedSlotable(const TSubclassOf<USlotable>& InSlotableClass) const;

	UCardObject* CreateUninitializedCardObject(const TSubclassOf<UCardObject>& InCardClass) const;

	void ClientCheckAndUpdateCardObjects();

	uint8 LastAssignedConstituentId = 0;

	uint8 ConstituentCount = 0;

	uint8 bIsOnFormCharacter:1;

	uint8 bInitialized:1;

	float GetCardLifetime(const TSubclassOf<UCardObject>& InCardClass, const int32 InOwnerConstituentInstanceId);
};
