// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory.h"

#include "Net/UnrealNetwork.h"

UInventory::UInventory()
{
	if (HasAuthority())
	{
		checkf(Capacity <= 128, TEXT("The capacity of an inventory may not exceed 128."))
		for (auto SlotableClass : InitialSlotableClasses)
		{
			USlotable* SlotableInstance = CreateUninitializedSlotable(SlotableClass);
			Slotables.Emplace(SlotableInstance);
			InitializeSlotable(SlotableInstance);
			MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
		}
	}
	else
	{
		ClientInitialize();
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
	if (HasAuthority())
	{
		for (int32 i = 0; i < Slotables.Num(); i++)
		{
			//We clear index 0 because the list shifts down.
			if (USlotable* Slotable = Slotables[0])
			{
				DeinitializeSlotable(Slotable);
				//We manually mark the object as garbage so its deletion can be replicated sooner to clients.
				Slotable->Destroy();
			}
			Slotables.RemoveAt(0);
			MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
		}
	}
	else
	{
		ClientDeinitialize();
	}
}

USlotable* UInventory::CreateUninitializedSlotable(const TSubclassOf<USlotable>& SlotableClass) const
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
	if (bRemoveSlot)
	{
		if (USlotable* Slotable = Slotables[Index])
		{
			DeinitializeSlotable(Slotable);
			//We manually mark the object as garbage so its deletion can be replicated sooner to clients.
			Slotable->Destroy();
		}
		Slotables.RemoveAt(Index);
	}
	else
	{
		SetSlotable(EmptySlotableClass, Index, false);
	}
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
}

USlotable* UInventory::SetSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index,
                                   const bool bSlotMustBeNullOrEmpty)
{
	ErrorIfNoAuthority();
	checkf(!bIsChangeLocked, TEXT("Attempted to set a slotable in a change locked inventory."));
	checkf(Index < 0 || Index >= Slotables.Num(), TEXT("Attempted to set a slotable with an index out of range."));
	USlotable* CurrentSlotable = Slotables[Index];
	if (CurrentSlotable)
	{
		if (bSlotMustBeNullOrEmpty)
		{
			checkf(CurrentSlotable->GetClass() == EmptySlotableClass,
			       TEXT("Attempted to set a slotable with an index out of range."));
		}
		DeinitializeSlotable(CurrentSlotable);
		//We manually mark the object as garbage so its deletion can be replicated sooner to clients.
		CurrentSlotable->Destroy();
	}
	USlotable* SlotableInstance = CreateUninitializedSlotable(SlotableClass);
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
	checkf(Slotables.Contains(SlotableA) && Slotables.Contains(SlotableB),
	       TEXT("Attempted to swap slotables between inventories."));
	SwapSlotablesByIndex(Slotables.Find(SlotableA), Slotables.Find(SlotableA));
	return true;
}

void UInventory::SwapSlotablesByIndex(const int32 IndexA, const int32 IndexB)
{
	ErrorIfNoAuthority();
	checkf(!bIsChangeLocked, TEXT("Attempted to swap slotables in a change locked inventory."));
	checkf(IndexA < 0 || IndexA >= Slotables.Num() || IndexB < 0 || IndexB >= Slotables.Num(),
	       TEXT("Attempted to swap slotables with an index out of range."));
	checkf(IndexA != IndexB, TEXT("Attempted to swap slotable with itself."));
	checkf(Slotables[IndexA] && Slotables[IndexB],
	       TEXT("Attempted to swap slotables with index referring to a nullptr"));
	DeinitializeSlotable(Slotables[IndexA]);
	DeinitializeSlotable(Slotables[IndexB]);
	USlotable* TempSlotablePtr = Slotables[IndexA];
	Slotables[IndexA] = Slotables[IndexB];
	Slotables[IndexB] = TempSlotablePtr;
	InitializeSlotable(Slotables[IndexA]);
	InitializeSlotable(Slotables[IndexB]);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
}

void UInventory::TradeSlotablesBetweenInventories(USlotable* SlotableA, USlotable* SlotableB)
{
	//Cannot be made static as authority check needs to be made.
	ErrorIfNoAuthority();
	checkf(SlotableA && SlotableB, TEXT("Attempted to trade slotables with a nullptr."));
	checkf(SlotableA != SlotableB, TEXT("Attempted to trade slotable with itself."));
	UInventory* InventoryA = SlotableA->OwningInventory;
	UInventory* InventoryB = SlotableB->OwningInventory;
	const int8 IndexA = InventoryA->GetSlotables().Find(SlotableA);
	const int8 IndexB = InventoryB->GetSlotables().Find(SlotableB);
	checkf(!InventoryA->bIsChangeLocked && !InventoryB->bIsChangeLocked,
	       TEXT("Attempted to trade slotables between change locked inventories."));
	DeinitializeSlotable(SlotableA);
	DeinitializeSlotable(SlotableB);
	InventoryA->Slotables[IndexA] = DuplicateObject(SlotableB, SlotableA->GetTypedOuter<AActor>());
	InventoryB->Slotables[IndexB] = DuplicateObject(SlotableA, SlotableB->GetTypedOuter<AActor>());
	//We manually mark the object as garbage so its deletion can be replicated sooner to clients.
	SlotableA->Destroy();
	SlotableB->Destroy();
	InitializeSlotable(InventoryA->Slotables[IndexA]);
	InitializeSlotable(InventoryB->Slotables[IndexB]);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, InventoryA);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, InventoryB);
}

void UInventory::ClientInitialize()
{
	//Call events.
}

void UInventory::ServerInitialize()
{
	//Call events.
}

void UInventory::ClientDeinitialize()
{
	//Call events.
}

void UInventory::ServerDeinitialize()
{
	//Call events.
}

//Must be called after the slotable has been placed in an inventory.
void UInventory::InitializeSlotable(USlotable* Slotable)
{
	GetOwner()->AddReplicatedSubObject(Slotable);
	Slotable->OwningInventory = this;
	MARK_PROPERTY_DIRTY_FROM_NAME(USlotable, OwningInventory, Slotable);
	Slotable->ServerInitialize();
}

//Must be called before the slotable is removed from an inventory.
void UInventory::DeinitializeSlotable(USlotable* Slotable)
{
	Slotable->ServerDeinitialize();
	Slotable->OwningInventory = nullptr;
	MARK_PROPERTY_DIRTY_FROM_NAME(USlotable, OwningInventory, Slotable);
	GetOwner()->RemoveReplicatedSubObject(Slotable);
}
