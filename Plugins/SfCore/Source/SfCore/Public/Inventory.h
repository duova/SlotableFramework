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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAddCard, FCard&, Card);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRemoveCard, FCard&, Card);

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

public:
	UInventory();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void AuthorityTick(float DeltaTime);
	
	UFUNCTION(BlueprintPure)
	const TArray<USlotable*>& GetSlotables();

	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere)
	class UFormCoreComponent* OwningFormCore;

	UFUNCTION(BlueprintPure)
	TArray<USlotable*> GetSlotablesOfType(const TSubclassOf<USlotable>& SlotableClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	USlotable* Server_AddSlotable(const TSubclassOf<USlotable>& SlotableClass, UConstituent* Origin);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_RemoveSlotable(USlotable* Slotable, const bool bRemoveSlot);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Server_RemoveSlotableByIndex(const int32 Index, const bool bRemoveSlot);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	USlotable* Server_SetSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index, const bool bSlotMustBeNullOrEmpty, UConstituent* Origin);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	USlotable* Server_InsertSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index, UConstituent* Origin);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_SwapSlotables(USlotable* SlotableA, USlotable* SlotableB);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Server_SwapSlotablesByIndex(const int32 IndexA, const int32 IndexB);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Server_TradeSlotablesBetweenInventories(USlotable* SlotableA, USlotable* SlotableB);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_AddSharedCard(const TSubclassOf<UCardObject>& CardClass, float CustomLifetime = 0);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_RemoveSharedCard(const TSubclassOf<UCardObject>& CardClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_AddOwnedCard(const TSubclassOf<UCardObject>& CardClass, const uint8 InOwnerConstituentInstanceId, float CustomLifetime = 0);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_RemoveOwnedCard(const TSubclassOf<UCardObject>& CardClass, const uint8 InOwnerConstituentInstanceId);

	UFUNCTION(BlueprintCallable)
	bool Predicted_AddSharedCard(const TSubclassOf<UCardObject>& CardClass, float CustomLifetime = 0);

	UFUNCTION(BlueprintCallable)
	bool Predicted_RemoveSharedCard(const TSubclassOf<UCardObject>& CardClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Predicted_AddOwnedCard(const TSubclassOf<UCardObject>& CardClass, const uint8 InOwnerConstituentInstanceId, float CustomLifetime = 0);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Predicted_RemoveOwnedCard(const TSubclassOf<UCardObject>& CardClass, const uint8 InOwnerConstituentInstanceId);

	UFUNCTION(BlueprintPure)
	bool HasSlotable(const TSubclassOf<USlotable>& SlotableClass);

	UFUNCTION(BlueprintPure)
	uint8 SlotableCount(const TSubclassOf<USlotable>& SlotableClass);

	UFUNCTION(BlueprintPure)
	bool HasSharedCard(const TSubclassOf<UCardObject>& CardClass);

	UFUNCTION(BlueprintPure)
	bool HasOwnedCard(const TSubclassOf<UCardObject>& CardClass, const uint8 InOwnerConstituentInstanceId);

	//Will return 0 if could not find or infinite lifetime.
	UFUNCTION(BlueprintPure)
	float GetSharedCardLifetime(const TSubclassOf<UCardObject>& CardClass);

	//Will return 0 if could not find or infinite lifetime.
	UFUNCTION(BlueprintPure)
	float GetOwnedCardLifetime(const TSubclassOf<UCardObject>& CardClass, const uint8 InOwnerConstituentInstanceId);

	//Only on client.
	UFUNCTION(BlueprintPure)
	TArray<UCardObject*> GetCardObjects(const TSubclassOf<UCardObject>& CardClass);

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

	void RemoveCardsOfOwner(const uint8 OwnerConstituentInstanceId);

	bool IsDynamicLength() const;

	UPROPERTY(BlueprintAssignable)
	FOnAddSlotable Server_OnAddSlotable;

	UPROPERTY(BlueprintAssignable)
	FOnRemoveSlotable Server_OnRemoveSlotable;

	UPROPERTY(BlueprintAssignable)
	FOnAddCard Server_OnAddCard;

	UPROPERTY(BlueprintAssignable)
	FOnRemoveCard Server_OnRemoveCard;

protected:

	virtual void BeginDestroy() override;

	//These must be registered with the FormCharacterComponent first.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<UInputAction*> OrderedInputBindings;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<USlotable>> InitialOrderedSlotableClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<UCardObject>> InitialSharedCardClassesInfiniteLifetime;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<USlotable> EmptySlotableClass;

	//Whether slotables can be added.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 bIsDynamic:1;

	//Whether the slotables in the inventory can be changed.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 bIsChangeLocked:1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 Capacity = uint8();

	/**
	 * Called after a slotable is added to an inventory.
	 */
	void InitializeSlotable(USlotable* Slotable, UConstituent* Origin);

	/**
	 * Called before a slotable is removed from an inventory.
	 */
	void DeinitializeSlotable(USlotable* Slotable);

private:

	UFUNCTION()
	void OnRep_Slotables();
	
	UPROPERTY(Replicated, VisibleAnywhere, ReplicatedUsing = OnRep_Slotables)
	TArray<USlotable*> Slotables;

	TArray<USlotable*> ClientSubObjectListRegisteredSlotables;

	TArray<int8> OrderedInputBindingIndices;

	TBitArray<> OrderedLastInputState;

	//Does not synchronize to owner as owner should have it be predicted.
	UPROPERTY(Replicated)
	TArray<FCard> Cards;

	UPROPERTY(VisibleAnywhere)
	TArray<UCardObject*> ClientCardObjects;
	
	USlotable* CreateUninitializedSlotable(const TSubclassOf<USlotable>& SlotableClass) const;

	UCardObject* CreateUninitializedCardObject(const TSubclassOf<UCardObject>& CardClass) const;

	void ClientCheckAndUpdateCardObjects();

	uint8 LastAssignedConstituentId;

	uint8 ConstituentCount;

	uint8 bIsOnFormCharacter:1;

	uint8 bInitialized:1;

	float GetCardLifetime(const TSubclassOf<UCardObject>& CardClass, int32 InOwnerConstituentInstanceId);
};
