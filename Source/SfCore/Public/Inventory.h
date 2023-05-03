// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfObject.h"
#include "Slotable.h"
#include "Inventory.generated.h"

/**
 * An inventory for slotables.
 * Can be dynamic or fixed size.
 */
UCLASS(Blueprintable)
class SFCORE_API UInventory : public USfObject
{
	GENERATED_BODY()

public:
	UInventory();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintGetter)
	TArray<USlotable*> GetSlotables();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	USlotable* AddSlotable(const TSubclassOf<USlotable>& SlotableClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool RemoveSlotable(USlotable* Slotable, const bool bRemoveSlot);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RemoveSlotableByIndex(const int32 Index, const bool bRemoveSlot);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	USlotable* SetSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	USlotable* InsertSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool SwapSlotables(USlotable* SlotableA, USlotable* SlotableB);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void SwapSlotablesByIndex(const int32 IndexA, const int32 IndexB);

protected:

	virtual void BeginDestroy() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<USlotable>> InitialSlotableClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<USlotable> EmptySlotableClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsDynamic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsChangeLocked;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Capacity;

	/*
	 * Called after a slotable is added to an inventory.
	 */
	void InitializeSlotable(USlotable* Slotable);

	/*
	 * Called before a slotable is removed from an inventory.
	 */
	void DeinitializeSlotable(USlotable* Slotable);

private:
	UPROPERTY(Replicated, VisibleAnywhere)
	TArray<USlotable*> Slotables;

	USlotable* CreateUninitializedSlotable(const TSubclassOf<USlotable>& SlotableClass);
};