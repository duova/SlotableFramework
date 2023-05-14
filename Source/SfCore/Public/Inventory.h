// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfObject.h"
#include "Slotable.h"
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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Read-only copy of constituents.
	 */
	UFUNCTION(BlueprintGetter)
	TArray<USlotable*> GetSlotables();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	USlotable* AddSlotable(const TSubclassOf<USlotable>& SlotableClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool RemoveSlotable(USlotable* Slotable, const bool bRemoveSlot);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RemoveSlotableByIndex(const int32 Index, const bool bRemoveSlot);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	USlotable* SetSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index, const bool bSlotMustBeNullOrEmpty);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	USlotable* InsertSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool SwapSlotables(USlotable* SlotableA, USlotable* SlotableB);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void SwapSlotablesByIndex(const int32 IndexA, const int32 IndexB);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void TradeSlotablesBetweenInventories(USlotable* SlotableA, USlotable* SlotableB);

	void ClientInitialize();

	void ServerInitialize();
	
	void ClientDeinitialize();

	void ServerDeinitialize();

protected:

	virtual void BeginDestroy() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<USlotable>> InitialSlotableClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<USlotable> EmptySlotableClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 bIsDynamic:1;

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

	USlotable* CreateUninitializedSlotable(const TSubclassOf<USlotable>& SlotableClass) const;
};
