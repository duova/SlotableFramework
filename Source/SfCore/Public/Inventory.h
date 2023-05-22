// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfObject.h"
#include "Slotable.h"
#include "Card.h"
#include "Inventory.generated.h"

/**
 * Inventories of slotables.
 * These can be dynamic or static, in terms of how many slotables they can contain.
 * They can also be preset with slotables, which can be locked.
 */
UCLASS(Blueprintable)
class SFCORE_API UInventory : public USfObject
{
	GENERATED_BODY()

public:
	UInventory();

	void Tick(float DeltaTime);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Read-only copy of slotables.
	 */
	UFUNCTION(BlueprintGetter)
	TArray<USlotable*> GetSlotables();

	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere)
	class UFormCoreComponent* OwningFormCore;

	UFUNCTION(BlueprintGetter)
	TArray<USlotable*> GetSlotablesOfType(const TSubclassOf<USlotable>& SlotableClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	USlotable* Server_AddSlotable(const TSubclassOf<USlotable>& SlotableClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_RemoveSlotable(USlotable* Slotable, const bool bRemoveSlot);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Server_RemoveSlotableByIndex(const int32 Index, const bool bRemoveSlot);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	USlotable* Server_SetSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index, const bool bSlotMustBeNullOrEmpty);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	USlotable* Server_InsertSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index);

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
	bool Server_AddOwnedCard(const TSubclassOf<UCardObject>& CardClass, const int32 InOwnerConstituentInstanceId, float CustomLifetime = 0);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_RemoveOwnedCard(const TSubclassOf<UCardObject>& CardClass, const int32 InOwnerConstituentInstanceId);

	UFUNCTION(BlueprintCallable)
	bool Predicted_AddSharedCard(const TSubclassOf<UCardObject>& CardClass, float CustomLifetime = 0);

	UFUNCTION(BlueprintCallable)
	bool Predicted_RemoveSharedCard(const TSubclassOf<UCardObject>& CardClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Predicted_AddOwnedCard(const TSubclassOf<UCardObject>& CardClass, const int32 InOwnerConstituentInstanceId, float CustomLifetime = 0);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Predicted_RemoveOwnedCard(const TSubclassOf<UCardObject>& CardClass, const int32 InOwnerConstituentInstanceId);

	UFUNCTION(BlueprintPure)
	bool HasSlotable(const TSubclassOf<USlotable>& SlotableClass);

	UFUNCTION(BlueprintPure)
	uint8 SlotableCount(const TSubclassOf<USlotable>& SlotableClass);

	UFUNCTION(BlueprintPure)
	bool HasSharedCard(const TSubclassOf<UCardObject>& CardClass);

	UFUNCTION(BlueprintPure)
	bool HasOwnedCard(const TSubclassOf<UCardObject>& CardClass, const int32 InOwnerConstituentInstanceId);

	//Will return 0 if could not find or infinite lifetime.
	UFUNCTION(BlueprintPure)
	float GetSharedCardLifetime(const TSubclassOf<UCardObject>& CardClass);

	//Will return 0 if could not find or infinite lifetime.
	UFUNCTION(BlueprintPure)
	float GetOwnedCardLifetime(const TSubclassOf<UCardObject>& CardClass, const int32 InOwnerConstituentInstanceId);

	//Only on client.
	UFUNCTION(BlueprintGetter)
	TArray<UCardObject*> GetCardObjects(const TSubclassOf<UCardObject>& CardClass);

	void ClientInitialize();

	void ServerInitialize();
	
	void ClientDeinitialize();

	void ServerDeinitialize();

	void AssignConstituentInstanceId(UConstituent* Constituent);

	void ReassignAllConstituentInstanceIds();

	void RemoveCardsOfOwner(const int32 OwnerConstituentInstanceId);

protected:

	virtual void BeginDestroy() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<USlotable>> InitialSlotableClasses;

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
	void InitializeSlotable(USlotable* Slotable);

	/**
	 * Called before a slotable is removed from an inventory.
	 */
	void DeinitializeSlotable(USlotable* Slotable);

private:
	UPROPERTY(Replicated, VisibleAnywhere)
	TArray<USlotable*> Slotables;

	UPROPERTY(VisibleAnywhere)
	TArray<UCardObject*> ClientCardObjects;

	//Does not synchronize to owner as owner should have it be predicted.
	UPROPERTY(Replicated)
	TArray<FCard> Cards;

	//Compares with this to update cards on change.
	TArray<FCard> ClientOldCards;

	USlotable* CreateUninitializedSlotable(const TSubclassOf<USlotable>& SlotableClass) const;

	UCardObject* CreateUninitializedCardObject(const TSubclassOf<UCardObject>& CardClass) const;

	void ClientCheckAndUpdateCardObjects();

	uint16 LastAssignedConstituentId;

	uint8 bIsOnFormCharacter:1;

	float GetCardLifetime(const TSubclassOf<UCardObject>& CardClass, int32 InOwnerConstituentInstanceId);
};
