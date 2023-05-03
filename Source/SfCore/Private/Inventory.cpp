// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory.h"

#include "Net/UnrealNetwork.h"

UInventory::UInventory()
{
	for (auto SlotableClass : InitialSlotableClasses)
	{
		AddSlotable(SlotableClass);
	}
}

void UInventory::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(UInventory, Slotables, DefaultParams);
}

void UInventory::BeginDestroy()
{
	UObject::BeginDestroy();
	for (int32 i = 0; i < Slotables.Num(); i++)
	{
		//We clear index 0 because the list shifts down.
		RemoveSlotableByIndex(0, true);
	}
}

USlotable* UInventory::CreateUninitializedSlotable(const TSubclassOf<USlotable>& SlotableClass)
{
	ErrorIfNoAuthority();
	if (!SlotableClass || SlotableClass->HasAnyClassFlags(CLASS_Abstract)) return nullptr;
	USlotable* SlotableInstance = NewObject<USlotable>(GetOwner(), SlotableClass);
	checkf(SlotableInstance, TEXT("Failed to create slotable."));
	return SlotableInstance;
}

TArray<USlotable*> UInventory::GetSlotables()
{
	TArray<USlotable*> SlotablesCopy;
	for (USlotable* Slotable : Slotables)
	{
		SlotablesCopy.Add(Slotable);
	}
	return SlotablesCopy;
}

USlotable* UInventory::AddSlotable(const TSubclassOf<USlotable>& SlotableClass)
{
	ErrorIfNoAuthority();
	checkf(bIsDynamic, TEXT("Attempted to add slotable to static inventory."));
	checkf(!bIsChangeLocked, TEXT("Attempted to add slotable to change locked inventory."));
	checkf(Slotables.Num() < Capacity, TEXT("Attempted to add slotable to a full inventory."));
	USlotable* SlotableInstance = CreateUninitializedSlotable(SlotableClass);
	Slotables.Emplace(SlotableInstance);
	InitializeSlotable(SlotableInstance);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
	return SlotableInstance;
}

bool UInventory::RemoveSlotable(USlotable* Slotable, const bool bRemoveSlot)
{
	ErrorIfNoAuthority();
	if (!Slotable) return false;
	const int32 Index = Slotables.Find(Slotable);
	if (Index == INDEX_NONE) return false;
	RemoveSlotableByIndex(Index, bRemoveSlot);
	return true;
}

void UInventory::RemoveSlotableByIndex(const int32 Index, const bool bRemoveSlot)
{
	ErrorIfNoAuthority();
	if (bRemoveSlot)
	{
		checkf(bIsDynamic, TEXT("Attempted to remove slotable from a static inventory."));
	}
	checkf(!bIsChangeLocked, TEXT("Attempted to remove slotable from a change locked inventory."));
	checkf(Index < 0 || Index >= Slotables.Num(), TEXT("Attempted to remove slotable with an index out of range."));
	USlotable* Slotable = Slotables[Index];
	if (Slotable)
	{
		DeinitializeSlotable(Slotable);
		//We manually mark the object as garbage so its deletion can be replicated sooner to clients.
		Slotable->Destroy();
	}
	if (bRemoveSlot)
	{
		Slotables.RemoveAt(Index);
	}
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
}

USlotable* UInventory::SetSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index)
{
	ErrorIfNoAuthority();
	checkf(!bIsChangeLocked, TEXT("Attempted to set a slotable in a change locked inventory."));
	checkf(Index < 0 || Index >= Slotables.Num(), TEXT("Attempted to set a slotable with an index out of range."));
	USlotable* SlotableInstance = CreateUninitializedSlotable(SlotableClass);
	RemoveSlotableByIndex(Index, false);
	Slotables[Index] = SlotableInstance;
	InitializeSlotable(SlotableInstance);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
	return SlotableInstance;
}

USlotable* UInventory::InsertSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index)
{
	ErrorIfNoAuthority();
	checkf(bIsDynamic, TEXT("Attempted to insert slotable into a static inventory."));
	checkf(!bIsChangeLocked, TEXT("Attempted to insert slotable into a change locked inventory."));
	checkf(Index < 0 || Index >= Slotables.Num(), TEXT("Attempted to insert slotable with an index out of range."));
	USlotable* SlotableInstance = CreateUninitializedSlotable(SlotableClass);
	Slotables.Insert(SlotableInstance, Index);
	InitializeSlotable(SlotableInstance);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
	return SlotableInstance;
}

bool UInventory::SwapSlotables(USlotable* SlotableA, USlotable* SlotableB)
{
	ErrorIfNoAuthority();
	if (!SlotableA || !SlotableB) return false;
	checkf(Slotables.Contains(SlotableA) && Slotables.Contains(SlotableB), TEXT("Attempted to swap slotables between inventories."));
	SwapSlotablesByIndex(Slotables.Find(SlotableA), Slotables.Find(SlotableA));
	return true;
}

void UInventory::SwapSlotablesByIndex(const int32 IndexA, const int32 IndexB)
{
	ErrorIfNoAuthority();
	checkf(!bIsChangeLocked, TEXT("Attempted to swap slotables in a change locked inventory."));
	checkf(IndexA < 0 || IndexA >= Slotables.Num() || IndexB < 0 || IndexB >= Slotables.Num(), TEXT("Attempted to swap slotables with an index out of range."));
	checkf(IndexA != IndexB, TEXT("Attempted to swap slotable with itself."));
	checkf(Slotables[IndexA] && Slotables[IndexB], TEXT("Attempted to swap slotables with index referring to a nullptr"));
	USlotable* TempSlotablePtr = Slotables[IndexA];
	Slotables[IndexA] = Slotables[IndexB];
	Slotables[IndexB] = TempSlotablePtr;
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
}

void UInventory::InitializeSlotable(USlotable* Slotable)
{
	GetOwner()->AddReplicatedSubObject(Slotable);
	//Call events.
}

void UInventory::DeinitializeSlotable(USlotable* Slotable)
{
	//Call events.
	GetOwner()->RemoveReplicatedSubObject(Slotable);
}
