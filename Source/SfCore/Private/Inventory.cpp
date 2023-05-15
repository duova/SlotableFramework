// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory.h"

#include "Net/UnrealNetwork.h"


FInventoryObjectMetadata::FInventoryObjectMetadata(const TSubclassOf<USlotable> SlotableClass):
	Type(), InputEnabledConstituentsCount(0)
{
	Class = SlotableClass;
	const USlotable* SlotableCDO = SlotableClass.GetDefaultObject();
	Type = EInventoryObjectType::Slotable;
	for (TSubclassOf<UConstituent> ConstituentClass : SlotableCDO->InitialConstituentClasses)
	{
		const UConstituent* ConstituentCDO = ConstituentClass.GetDefaultObject();
		if (ConstituentCDO->bEnableInputsAndPrediction)
		{
			InputEnabledConstituentsCount++;
		}
	}
}

FInventoryObjectMetadata::FInventoryObjectMetadata(const TSubclassOf<UCard> CardClass):
	Type(), InputEnabledConstituentsCount(0)
{
	Type = EInventoryObjectType::Card;
	Class = CardClass;
}

UInventory::UInventory()
{
	if (HasAuthority())
	{
		checkf(Capacity <= 128, TEXT("The capacity of an inventory may not exceed 128."))
		//Make sure we fill slotable metadata before card metadata.
		for (TSubclassOf<USlotable> SlotableClass : InitialSlotableClasses)
		{
			USlotable* SlotableInstance = CreateUninitializedSlotable(SlotableClass);
			Slotables.Emplace(SlotableInstance);
			Metadata.Emplace(FInventoryObjectMetadata(SlotableClass));
			InitializeSlotable(SlotableInstance);
		}
		for (uint8 i = 0; i < InitialCardClasses.Num(); i++)
		{
			for (uint8 j = 0; j < InitialCardClasses.Num(); j++)
			{
				checkf(i == j || InitialCardClasses[i] != InitialCardClasses[j],
				       TEXT("Each card in an inventory must be unique."));
			}
		}
		for (const TSubclassOf<UCard> CardClass : InitialCardClasses)
		{
			Metadata.Emplace(FInventoryObjectMetadata(CardClass));
		}
		MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
		MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Metadata, this);
	}
	else
	{
		//We can directly init here because when the constructor is called, the object should have a reference to its outer.
		ClientInitialize();
	}
}

void UInventory::Tick(float DeltaTime)
{
	CheckAndUpdateCardsWithMetadata();
}

void UInventory::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(UInventory, Slotables, DefaultParams);
	FDoRepLifetimeParams MetaDataParams;
	MetaDataParams.bIsPushBased = true;
	MetaDataParams.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(UInventory, Metadata, MetaDataParams)
}

void UInventory::BeginDestroy()
{
	UObject::BeginDestroy();
	if (HasAuthority())
	{
		for (uint8 i = 0; i < Slotables.Num(); i++)
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
		Metadata.Empty();
		MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Metadata, this);
	}
	else
	{
		for (uint8 i = 0; i < Cards.Num(); i++)
		{
			//We clear index 0 because the list shifts down.
			if (UCard* Card = Cards[0])
			{
				Card->Deinitialize();
			}
			Cards.RemoveAt(0);
		}
		//We empty the metadata so the cards do not get created again.
		Metadata.Empty();
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

UCard* UInventory::CreateUninitializedCard(const TSubclassOf<UCard>& CardClass) const
{
	if (!CardClass || CardClass->HasAnyClassFlags(CLASS_Abstract)) return nullptr;
	UCard* CardInstance = NewObject<UCard>(GetOwner(), CardClass);
	checkf(CardInstance, TEXT("Failed to create card."));
	return CardInstance;
}

void UInventory::CheckAndUpdateCardsWithMetadata()
{
	TArray<UCard*> ToRemove;
	for (UCard* Card : Cards)
	{
		bool bHasMetadata = false;
		for (FInventoryObjectMetadata Element : Metadata)
		{
			if (Element.Type != EInventoryObjectType::Card) continue;
			if (Element.Class == Card->GetClass()) bHasMetadata = true;
		}
		if (!bHasMetadata)
		{
			ToRemove.Add(Card);
		}
	}
	for (UCard* InvalidCard : ToRemove)
	{
		InvalidCard->Deinitialize();
		Cards.Remove(InvalidCard);
	}
	for (FInventoryObjectMetadata Element : Metadata)
	{
		if (Element.Type != EInventoryObjectType::Card) continue;
		bool bHasCard = false;
		for (UCard* Card : Cards)
		{
			if (Element.Class == Card->GetClass()) bHasCard = true;
		}
		if (bHasCard) continue;
		TSubclassOf<UCard> CardClass{Element.Class};
		UCard* CardInstance = CreateUninitializedCard(CardClass);
		Cards.Emplace(CardInstance);
		CardInstance->OwningInventory = this;
		CardInstance->Initialize();
	}
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

TArray<USlotable*> UInventory::GetSlotablesOfType(const TSubclassOf<USlotable>& SlotableClass)
{
	TArray<USlotable*> SlotablesCopy;
	for (USlotable* Slotable : Slotables)
	{
		if (Slotable->GetClass() == SlotableClass)
		{
			SlotablesCopy.Add(Slotable);
		}
	}
	return SlotablesCopy;
}

bool UInventory::HasSlotable(const TSubclassOf<USlotable>& SlotableClass)
{
	bool Value = false;
	for (FInventoryObjectMetadata Element : Metadata)
	{
		if (Element.Type == EInventoryObjectType::Slotable && Element.Class == SlotableClass) Value = true;
	}
	return Value;
}

uint8 UInventory::SlotableCount(const TSubclassOf<USlotable>& SlotableClass)
{
	
}

bool UInventory::HasCard(const TSubclassOf<USlotable>& CardClass)
{
}

bool UInventory::GetCard(const TSubclassOf<USlotable>& CardClass)
{
}

void UInventory::InsertSlotableMetadataAtEnd(const TSubclassOf<USlotable>& SlotableClass)
{
	uint8 Index = 255;
	for (uint8 i = 0; i < Metadata.Num(); i++)
	{
		if (Metadata[i].Type != EInventoryObjectType::Slotable)
		{
			Index = i;
		}
	}
	if (Index == 255)
	{
		Metadata.Insert(FInventoryObjectMetadata(SlotableClass), Metadata.Num() - 1);
	}
	else
	{
		Metadata.Insert(FInventoryObjectMetadata(SlotableClass), Index);
	}
}

USlotable* UInventory::Server_AddSlotable(const TSubclassOf<USlotable>& SlotableClass)
{
	ErrorIfNoAuthority();
	checkf(bIsDynamic, TEXT("Attempted to add slotable to static inventory."));
	checkf(!bIsChangeLocked, TEXT("Attempted to add slotable to change locked inventory."));
	checkf(Slotables.Num() < Capacity, TEXT("Attempted to add slotable to a full inventory."));
	USlotable* SlotableInstance = CreateUninitializedSlotable(SlotableClass);
	Slotables.Emplace(SlotableInstance);
	InsertSlotableMetadataAtEnd(SlotableClass);
	InitializeSlotable(SlotableInstance);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Metadata, this);
	return SlotableInstance;
}

bool UInventory::Server_RemoveSlotable(USlotable* Slotable, const bool bRemoveSlot)
{
	ErrorIfNoAuthority();
	if (!Slotable) return false;
	const int32 Index = Slotables.Find(Slotable);
	if (Index == INDEX_NONE) return false;
	Server_RemoveSlotableByIndex(Index, bRemoveSlot);
	return true;
}

void UInventory::Server_RemoveSlotableByIndex(const int32 Index, const bool bRemoveSlot)
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
		Metadata.RemoveAt(Index);
	}
	else
	{
		Server_SetSlotable(EmptySlotableClass, Index, false);
	}
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Metadata, this);
}

USlotable* UInventory::Server_SetSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index,
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
			       TEXT("Attempted to set a slotable to a slot that is occupied."));
		}
		DeinitializeSlotable(CurrentSlotable);
		//We manually mark the object as garbage so its deletion can be replicated sooner to clients.
		CurrentSlotable->Destroy();
	}
	USlotable* SlotableInstance = CreateUninitializedSlotable(SlotableClass);
	Slotables[Index] = SlotableInstance;
	Metadata[Index] = FInventoryObjectMetadata(SlotableClass);
	InitializeSlotable(SlotableInstance);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Metadata, this);
	return SlotableInstance;
}

USlotable* UInventory::Server_InsertSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index)
{
	ErrorIfNoAuthority();
	checkf(bIsDynamic, TEXT("Attempted to insert slotable into a static inventory."));
	checkf(!bIsChangeLocked, TEXT("Attempted to insert slotable into a change locked inventory."));
	checkf(Index < 0 || Index >= Slotables.Num(), TEXT("Attempted to insert slotable with an index out of range."));
	USlotable* SlotableInstance = CreateUninitializedSlotable(SlotableClass);
	Slotables.Insert(SlotableInstance, Index);
	Metadata.Insert(FInventoryObjectMetadata(SlotableClass), Index);
	InitializeSlotable(SlotableInstance);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Metadata, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
	return SlotableInstance;
}

bool UInventory::Server_SwapSlotables(USlotable* SlotableA, USlotable* SlotableB)
{
	ErrorIfNoAuthority();
	if (!SlotableA || !SlotableB) return false;
	checkf(Slotables.Contains(SlotableA) && Slotables.Contains(SlotableB),
	       TEXT("Attempted to swap slotables between inventories."));
	Server_SwapSlotablesByIndex(Slotables.Find(SlotableA), Slotables.Find(SlotableA));
	return true;
}

void UInventory::Server_SwapSlotablesByIndex(const int32 IndexA, const int32 IndexB)
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
	Metadata.Swap(IndexA, IndexB);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Metadata, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
}

void UInventory::Server_TradeSlotablesBetweenInventories(USlotable* SlotableA, USlotable* SlotableB)
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
	FInventoryObjectMetadata DataA = InventoryA->Metadata[IndexA];
	InventoryA->Metadata[IndexA] = InventoryB->Metadata[IndexB];
	InventoryB->Metadata[IndexB] = DataA;
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Metadata, InventoryA);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Metadata, InventoryB);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, InventoryA);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, InventoryB);
}

bool UInventory::Server_AddCard(const TSubclassOf<UCard>& CardClass)
{
	//We only need to add the metadata as the client will take care of the rest.
	const FInventoryObjectMetadata Data = FInventoryObjectMetadata(CardClass);
	if (Metadata.Contains(Data)) return false;
	Metadata.Add(Data);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Metadata, this);
	return true;
}

bool UInventory::Server_RemoveCard(const TSubclassOf<UCard>& CardClass)
{
	//We only need to add the metadata as the client will take care of the rest.
	const FInventoryObjectMetadata Data = FInventoryObjectMetadata(CardClass);
	if (!Metadata.Contains(Data)) return false;
	Metadata.Remove(Data);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Metadata, this);
	return true;
}

//Predicted functions essentially have the same implementations but on client, they only make changes to Metadata.

bool UInventory::Predicted_AddSlotable(const TSubclassOf<USlotable>& SlotableClass)
{
	checkf(GetOwner()->GetLocalRole() != ROLE_SimulatedProxy, TEXT("Called Predicted_ function on simulated proxy."))
	if (!SlotableClass) return false;
	if (HasAuthority())
	{
		Server_AddSlotable(SlotableClass);
	}
	else
	{
		checkf(bIsDynamic, TEXT("Attempted to add slotable to static inventory."));
		checkf(!bIsChangeLocked, TEXT("Attempted to add slotable to change locked inventory."));
		checkf(Slotables.Num() < Capacity, TEXT("Attempted to add slotable to a full inventory."));
		InsertSlotableMetadataAtEnd(SlotableClass);
	}
	return true;
}

bool UInventory::Predicted_RemoveSlotable(USlotable* Slotable, const bool bRemoveSlot)
{
	checkf(GetOwner()->GetLocalRole() != ROLE_SimulatedProxy, TEXT("Called Predicted_ function on simulated proxy."))
	if (!Slotable) return false;
	if (HasAuthority())
	{
		Server_RemoveSlotable(Slotable, bRemoveSlot);
	}
	else
	{
		const int32 Index = Slotables.Find(Slotable);
		if (Index == INDEX_NONE) return false;
		if (bRemoveSlot)
		{
			checkf(bIsDynamic, TEXT("Attempted to remove slotable from a static inventory."));
		}
		checkf(!bIsChangeLocked, TEXT("Attempted to remove slotable from a change locked inventory."));
		checkf(Index < 0 || Index >= Slotables.Num(), TEXT("Attempted to remove slotable with an index out of range."));
		if (bRemoveSlot)
		{
			Metadata.RemoveAt(Index);
		}
		else
		{
			checkf(!bIsChangeLocked, TEXT("Attempted to set a slotable in a change locked inventory."));
			checkf(Index < 0 || Index >= Slotables.Num(),
			       TEXT("Attempted to set a slotable with an index out of range."));
			Metadata[Index] = FInventoryObjectMetadata(EmptySlotableClass);
		}
	}
	return true;
}

void UInventory::Predicted_RemoveSlotableByIndex(const int32 Index, const bool bRemoveSlot)
{
	checkf(GetOwner()->GetLocalRole() != ROLE_SimulatedProxy, TEXT("Called Predicted_ function on simulated proxy."))
	if (HasAuthority())
	{
		Server_RemoveSlotableByIndex(Index, bRemoveSlot);
	}
	else
	{
		if (bRemoveSlot)
		{
			checkf(bIsDynamic, TEXT("Attempted to remove slotable from a static inventory."));
		}
		checkf(!bIsChangeLocked, TEXT("Attempted to remove slotable from a change locked inventory."));
		checkf(Index < 0 || Index >= Slotables.Num(), TEXT("Attempted to remove slotable with an index out of range."));
		if (bRemoveSlot)
		{
			Metadata.RemoveAt(Index);
		}
		else
		{
			checkf(!bIsChangeLocked, TEXT("Attempted to set a slotable in a change locked inventory."));
			checkf(Index < 0 || Index >= Slotables.Num(),
			       TEXT("Attempted to set a slotable with an index out of range."));
			Metadata[Index] = FInventoryObjectMetadata(EmptySlotableClass);
		}
	}
}

bool UInventory::Predicted_SetSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index,
                                       const bool bSlotMustBeNullOrEmpty)
{
	checkf(GetOwner()->GetLocalRole() != ROLE_SimulatedProxy, TEXT("Called Predicted_ function on simulated proxy."))
	if (!SlotableClass) return false;
	if (HasAuthority())
	{
		Server_SetSlotable(SlotableClass, Index, bSlotMustBeNullOrEmpty);
	}
	else
	{
		checkf(!bIsChangeLocked, TEXT("Attempted to set a slotable in a change locked inventory."));
		checkf(Index < 0 || Index >= Slotables.Num(), TEXT("Attempted to set a slotable with an index out of range."));
		USlotable* CurrentSlotable = Slotables[Index];
		if (bSlotMustBeNullOrEmpty)
		{
			checkf(CurrentSlotable->GetClass() == EmptySlotableClass,
			       TEXT("Attempted to set a slotable to a slot that is occupied."));
		}
		Metadata[Index] = FInventoryObjectMetadata(SlotableClass);
	}
	return true;
}

bool UInventory::Predicted_InsertSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index)
{
	checkf(GetOwner()->GetLocalRole() != ROLE_SimulatedProxy, TEXT("Called Predicted_ function on simulated proxy."))
	if (!SlotableClass) return false;
	if (HasAuthority())
	{
		Server_InsertSlotable(SlotableClass, Index);
	}
	else
	{
		checkf(bIsDynamic, TEXT("Attempted to insert slotable into a static inventory."));
		checkf(!bIsChangeLocked, TEXT("Attempted to insert slotable into a change locked inventory."));
		checkf(Index < 0 || Index >= Slotables.Num(), TEXT("Attempted to insert slotable with an index out of range."));
		Metadata.Insert(FInventoryObjectMetadata(SlotableClass), Index);
	}
	return true;
}

bool UInventory::Predicted_AddCard(const TSubclassOf<UCard>& CardClass)
{
	checkf(GetOwner()->GetLocalRole() != ROLE_SimulatedProxy, TEXT("Called Predicted_ function on simulated proxy."))
	if (!CardClass) return false;
	if (HasAuthority())
	{
		return Server_AddCard(CardClass);
	}
	const FInventoryObjectMetadata Data = FInventoryObjectMetadata(CardClass);
	if (Metadata.Contains(Data)) return false;
	Metadata.Add(Data);
	UCard* CardInstance = CreateUninitializedCard(CardClass);
	CardInstance->Initialize();
	return true;
}

bool UInventory::Predicted_RemoveCard(const TSubclassOf<UCard>& CardClass)
{
	checkf(GetOwner()->GetLocalRole() != ROLE_SimulatedProxy, TEXT("Called Predicted_ function on simulated proxy."))
	if (!CardClass) return false;
	if (HasAuthority())
	{
		return Server_RemoveCard(CardClass);
	}
	const FInventoryObjectMetadata Data = FInventoryObjectMetadata(CardClass);
	if (!Metadata.Contains(Data)) return false;
	Metadata.Remove(Data);
	UCard* ToRemove;
	for (UCard* Card : Cards)
	{
		if (Card->GetClass() == CardClass)
		{
			Card->Deinitialize();
			ToRemove = Card;
		}
	}
	Cards.Remove(ToRemove);
	return true;
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
