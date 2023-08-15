// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory.h"

#include "CardObject.h"
#include "FormCharacterComponent.h"
#include "FormCoreComponent.h"
#include "Slotable.h"
#include "Net/UnrealNetwork.h"

UInventory::UInventory()
{
	if (!GetOwner()) return;
	bIsOnFormCharacter = false;
	bInitialized = false;
}

void UInventory::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(UInventory, Slotables, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UInventory, OwningFormCore, DefaultParams);
	FDoRepLifetimeParams CardParams;
	CardParams.bIsPushBased = true;
	CardParams.Condition = COND_None;
	//We handle owner replication for cards purely with the FormCharacterComponent if it is available.
	if (GetOwner() && GetOwner()->FindComponentByClass(UFormCharacterComponent::StaticClass()))
	{
		CardParams.Condition = COND_SkipOwner;
	}
	DOREPLIFETIME_WITH_PARAMS_FAST(UInventory, Cards, CardParams);
}

void UInventory::AuthorityTick(float DeltaTime)
{
	//We remove server timestamp cards with ended lifetimes only on the server.
	//This is synchronize to clients through the FormCharacter and normal replication depending on the role of the client.
	for (int16 i = Cards.Num() - 1; i >= 0; i--)
	{
		if (!Cards[i].bUsingPredictedTimestamp && OwningFormCore->CalculateTimeUntilServerTimestamp(
			Cards[i].LifetimeEndTimestamp) < 0)
		{
			Server_RemoveOwnedCard(Cards[i].Class, Cards[i].OwnerConstituentInstanceId);
		}
	}
}

void UInventory::BeginDestroy()
{
	UObject::BeginDestroy();
	if (!GetOwner()) return;
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
			Slotables.RemoveAt(0, 1, false);
			MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
		}
		Slotables.Shrink();
		Cards.Empty();
		MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Cards, this);
	}
	else
	{
		for (uint8 i = 0; i < ClientCardObjects.Num(); i++)
		{
			//We clear index 0 because the list shifts down.
			if (UCardObject* Card = ClientCardObjects[0])
			{
				Card->Deinitialize();
			}
			ClientCardObjects.RemoveAt(0, 1, false);
		}
		ClientCardObjects.Shrink();
		//We empty the list so the cards do not get created again.
		Cards.Empty();
	}
}

USlotable* UInventory::CreateUninitializedSlotable(const TSubclassOf<USlotable>& InSlotableClass) const
{
	if (!GetOwner()) return nullptr;
	if (!HasAuthority())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called CreateUninitializedSlotable on UInventory class %s without authority."),
		       *GetClass()->GetName());
		return nullptr;
	}
	if (!InSlotableClass.Get())
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called CreateUninitializedSlotable on UInventory class %s with an empty TSubclassOf."),
		       *GetClass()->GetName());
		return nullptr;
	}
	if (!InSlotableClass || InSlotableClass->HasAnyClassFlags(CLASS_Abstract)) return nullptr;
	USlotable* SlotableInstance = NewObject<USlotable>(GetOwner(), InSlotableClass);
	if (!SlotableInstance)
	{
		UE_LOG(LogSfCore, Error, TEXT("Failed to create USlotable on Inventory class %s."), *GetClass()->GetName());
	}
	return SlotableInstance;
}

UCardObject* UInventory::CreateUninitializedCardObject(const TSubclassOf<UCardObject>& InCardClass) const
{
	if (!GetOwner()) return nullptr;
	if (!InCardClass.Get())
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called CreateUninitializedCardObject on UInventory class %s with an empty TSubclassOf."),
		       *GetClass()->GetName());
		return nullptr;
	}
	if (!InCardClass || InCardClass->HasAnyClassFlags(CLASS_Abstract)) return nullptr;
	UCardObject* CardInstance = NewObject<UCardObject>(GetOwner(), InCardClass);
	if (!CardInstance)
	{
		UE_LOG(LogSfCore, Error, TEXT("Failed to create UCardObject on UInventory class %s."), *GetClass()->GetName());
	}
	return CardInstance;
}

void UInventory::ClientCheckAndUpdateCardObjects()
{
	//Try to remove card objects that aren't on the cards list.
	for (int16 i = ClientCardObjects.Num() - 1; i >= 0; i--)
	{
		bool bHasCard = false;
		for (FCard& Card : Cards)
		{
			//Cards marked disabled for destroy don't count.
			if (Card.bIsDisabledForDestroy) continue;
			if (Card.Class == ClientCardObjects[i]->GetClass() && Card.OwnerConstituentInstanceId == ClientCardObjects[
					i]->
				OwnerConstituentInstanceId)
			{
				bHasCard = true;
				break;
			}
		}
		if (!bHasCard)
		{
			ClientCardObjects[i]->Deinitialize();
			ClientCardObjects.RemoveAtSwap(i, 1, false);
		}
	}
	ClientCardObjects.Shrink();
	//Try to add card objects that are on the cards list but doesn't exist.
	for (FCard Card : Cards)
	{
		if (Card.bIsDisabledForDestroy) continue;
		bool bHasCard = false;
		for (const UCardObject* CardObject : ClientCardObjects)
		{
			if (Card.Class == CardObject->GetClass() && Card.OwnerConstituentInstanceId == CardObject->
				OwnerConstituentInstanceId)
			{
				bHasCard = true;
				break;
			}
		}
		if (bHasCard) continue;
		//Continue if this card object is not supposed to be spawned.
		if (!Card.Class.GetDefaultObject()->bSpawnCardObjectOnClient) continue;
		TSubclassOf<UCardObject> CardClass = Card.Class;
		UCardObject* CardInstance = CreateUninitializedCardObject(CardClass);
		ClientCardObjects.Add(CardInstance);
		CardInstance->OwningInventory = this;
		CardInstance->OwnerConstituentInstanceId = Card.OwnerConstituentInstanceId;
		CardInstance->Initialize();
	}
}

const TArray<USlotable*>& UInventory::GetSlotables() const
{
	return Slotables;
}

TArray<USlotable*> UInventory::GetSlotablesOfClass(const TSubclassOf<USlotable>& InSlotableClass) const
{
	if (!InSlotableClass.Get())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called GetSlotablesOfClass on UInventory class %s with an empty TSubclassOf."),
		       *GetClass()->GetName());
		return TArray<USlotable*>();
	}
	TArray<USlotable*> SlotablesOfClass;
	for (USlotable* Slotable : Slotables)
	{
		if (Slotable->GetClass() == InSlotableClass)
		{
			SlotablesOfClass.Add(Slotable);
		}
	}
	return SlotablesOfClass;
}

bool UInventory::HasSlotableOfClass(const TSubclassOf<USlotable>& InSlotableClass)
{
	if (!InSlotableClass.Get())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called HasSlotableOfClass on UInventory class %s with an empty TSubclassOf."),
		       *GetClass()->GetName());
		return false;
	}
	bool Value = false;
	for (const USlotable* Slotable : Slotables)
	{
		if (Slotable->GetClass() == InSlotableClass)
		{
			Value = true;
			break;
		}
	}
	return Value;
}

int32 UInventory::SlotableCount(const TSubclassOf<USlotable>& InSlotableClass)
{
	if (!InSlotableClass.Get())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called SlotableCount on UInventory class %s with an empty TSubclassOf."),
		       *GetClass()->GetName());
		return 0;
	}
	uint8 Value = 0;
	for (const USlotable* Slotable : Slotables)
	{
		if (Slotable->GetClass() == InSlotableClass) Value++;
	}
	return Value;
}

bool UInventory::HasSharedCard(const TSubclassOf<UCardObject>& InCardClass) const
{
	if (!InCardClass.Get())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called HasSharedCard on UInventory class %s with an empty TSubclassOf."),
		       *GetClass()->GetName());
		return false;
	}
	bool Value = false;
	for (const FCard& Card : Cards)
	{
		//If disabled we don't count it as existing.
		if (Card.bIsDisabledForDestroy) continue;
		if (Card.Class == InCardClass && Card.OwnerConstituentInstanceId == 0)
		{
			Value = true;
			break;
		}
	}
	return Value;
}

bool UInventory::HasOwnedCard(const TSubclassOf<UCardObject>& InCardClass,
                              const int32 InOwnerConstituentInstanceId) const
{
	if (!InCardClass.Get())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called HasOwnedCard on UInventory class %s with an empty TSubclassOf."),
		       *GetClass()->GetName());
		return nullptr;
	}
	bool Value = false;
	for (FCard Card : Cards)
	{
		//If disabled we don't count it as existing.
		if (Card.bIsDisabledForDestroy) continue;
		if (Card.Class == InCardClass && Card.OwnerConstituentInstanceId == InOwnerConstituentInstanceId)
		{
			Value = true;
			break;
		}
	}
	return Value;
}

float UInventory::GetSharedCardLifetime(const TSubclassOf<UCardObject>& InCardClass)
{
	if (!InCardClass.Get())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called GetSharedCardLifetime on UInventory class %s with an empty TSubclassOf."),
		       *GetClass()->GetName());
		return 0;
	}
	return GetCardLifetime(InCardClass, 0);
}

float UInventory::GetOwnedCardLifetime(const TSubclassOf<UCardObject>& InCardClass,
                                       const int32 InOwnerConstituentInstanceId)
{
	if (!InCardClass.Get())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called GetOwnedCardLifetime on UInventory class %s with an empty TSubclassOf."),
		       *GetClass()->GetName());
		return 0;
	}
	return GetCardLifetime(InCardClass, InOwnerConstituentInstanceId);
}

const TArray<UCardObject*>& UInventory::Client_GetCardObjects() const
{
	if (HasAuthority())
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called GetCardObjects on UInventory class %s on the server. UCardObjects only spawn on the client."
		       ), *GetClass()->GetName());
	}
	return ClientCardObjects;
}

int32 UInventory::GetRemainingCapacity() const
{
	return Capacity - Slotables.Num();
}

USlotable* UInventory::Server_AddSlotable(const TSubclassOf<USlotable>& InSlotableClass, UConstituent* Origin)
{
	if (!HasAuthority())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called Server_AddSlotable on UInventory class %s without authority."),
		       *GetClass()->GetName());
		return nullptr;
	}
	if (!InSlotableClass.Get())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called Server_AddSlotable on UInventory class %s with an empty TSubclassOf."),
		       *GetClass()->GetName());
		return nullptr;
	}
	if (!bIsDynamic)
	{
		UE_LOG(LogSfCore, Error, TEXT("Called Server_AddSlotable on UInventory class %s set to be a static inventory."),
		       *GetClass()->GetName());
		return nullptr;
	}
	if (bIsChangeLocked)
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called Server_AddSlotable on UInventory class %s set to be a change-locked inventory."),
		       *GetClass()->GetName());
		return nullptr;
	}
	if (Slotables.Num() >= Capacity)
	{
		UE_LOG(LogSfCore, Error, TEXT("Called Server_AddSlotable on full UInventory class %s."),
		       *GetClass()->GetName());
		return nullptr;
	}
	if (ConstituentCount > 254 - InSlotableClass.GetDefaultObject()->InitialConstituentClasses.Num())
	{
		UE_LOG(LogTemp, Error,
		       TEXT("Called Server_AddSlotable on UInventory class %s with max constituent count of 254 reached."), *GetClass()->GetName());
		return nullptr;
	}
	USlotable* SlotableInstance = CreateUninitializedSlotable(InSlotableClass);
	Slotables.Add(SlotableInstance);
	SlotableInstance->OwningInventory = this;
	InitializeSlotable(SlotableInstance, Origin);
	ConstituentCount += SlotableInstance->GetConstituents().Num();
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
	CallBindedOnAddSlotableDelegates(SlotableInstance);
	return SlotableInstance;
}

bool UInventory::Server_RemoveSlotable(USlotable* Slotable, const bool bInRemoveSlot)
{
	if (!HasAuthority())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called Server_RemoveSlotable on UInventory class %s without authority."),
		       *GetClass()->GetName());
		return false;
	}
	if (!Slotable) return false;
	const int32 Index = Slotables.Find(Slotable);
	if (Index == INDEX_NONE) return false;
	Server_RemoveSlotableByIndex(Index, bInRemoveSlot);
	return true;
}

void UInventory::Server_RemoveSlotableByIndex(const int32 InIndex, const bool bInRemoveSlot)
{
	if (!HasAuthority())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called Server_RemoveSlotableByIndex on UInventory class %s without authority."),
		       *GetClass()->GetName());
		return;
	}
	if (bIsChangeLocked)
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called Server_RemoveSlotableByIndex on UInventory class %s set to be a change-locked inventory."),
		       *GetClass()->GetName());
		return;
	}
	if (InIndex < 0 || InIndex >= Slotables.Num())
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called Server_RemoveSlotableByIndex on UInventory class %s with an index out of range."),
		       *GetClass()->GetName());
		return;
	}
	if (bInRemoveSlot)
	{
		if (!bIsDynamic)
		{
			UE_LOG(LogSfCore, Error,
			       TEXT(
				       "Called Server_RemoveSlotableByIndex with slot removal on UInventory class %s set to be a static inventory."
			       ), *GetClass()->GetName());
			return;
		}
		if (USlotable* Slotable = Slotables[InIndex])
		{
			CallBindedOnRemoveSlotableDelegates(Slotable);
			ConstituentCount -= Slotable->GetConstituents().Num();
			DeinitializeSlotable(Slotable);
			//We manually mark the object as garbage so its deletion can be replicated sooner to clients.
			Slotable->Destroy();
		}
		Slotables.RemoveAt(InIndex);
	}
	else
	{
		if (USlotable* Slotable = Slotables[InIndex])
		{
			CallBindedOnRemoveSlotableDelegates(Slotable);
		}
		Server_SetSlotable(EmptySlotableClass, InIndex, false, nullptr);
	}
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
}

USlotable* UInventory::Server_SetSlotable(const TSubclassOf<USlotable>& InSlotableClass, const int32 InIndex,
                                          const bool bInSlotMustBeNullOrEmpty, UConstituent* Origin)
{
	if (!HasAuthority())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called Server_SetSlotable on UInventory class %s without authority."),
		       *GetClass()->GetName());
		return nullptr;
	}
	if (bIsChangeLocked)
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called Server_SetSlotable on UInventory class %s set to be a change-locked inventory."),
		       *GetClass()->GetName());
		return nullptr;
	}
	if (InIndex < 0 || InIndex >= Slotables.Num())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called Server_SetSlotable on UInventory class %s with an index out of range."),
		       *GetClass()->GetName());
		return nullptr;
	}
	USlotable* CurrentSlotable = Slotables[InIndex];
	if (CurrentSlotable)
	{
		if (bInSlotMustBeNullOrEmpty)
		{
			if (CurrentSlotable->GetClass() != EmptySlotableClass)
			{
				UE_LOG(LogSfCore, Error,
				       TEXT(
					       "Called Server_SetSlotable on UInventory class %s targeting an occupied slot with bInSlotMustBeNullOrEmpty true."
				       ), *GetClass()->GetName());
				return nullptr;
			}
		}
		CallBindedOnRemoveSlotableDelegates(CurrentSlotable);
		DeinitializeSlotable(CurrentSlotable);
		//We manually mark the object as garbage so its deletion can be replicated sooner to clients.
		CurrentSlotable->Destroy();
	}
	USlotable* SlotableInstance = CreateUninitializedSlotable(InSlotableClass);
	Slotables[InIndex] = SlotableInstance;
	InitializeSlotable(SlotableInstance, Origin);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
	CallBindedOnAddSlotableDelegates(SlotableInstance);
	return SlotableInstance;
}

USlotable* UInventory::Server_InsertSlotable(const TSubclassOf<USlotable>& InSlotableClass, const int32 InIndex,
                                             UConstituent* Origin)
{
	if (!HasAuthority())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called Server_InsertSlotable on UInventory class %s without authority."),
		       *GetClass()->GetName());
		return nullptr;
	}
	if (!bIsDynamic)
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called Server_InsertSlotable on UInventory class %s set to be a static inventory."),
		       *GetClass()->GetName());
		return nullptr;
	}
	if (bIsChangeLocked)
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called Server_InsertSlotable on UInventory class %s set to be a change-locked inventory."),
		       *GetClass()->GetName());
		return nullptr;
	}
	if (InIndex < 0 || InIndex >= Slotables.Num())
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called Server_InsertSlotable on UInventory class %s with an index out of range."),
		       *GetClass()->GetName());
		return nullptr;
	}
	USlotable* SlotableInstance = CreateUninitializedSlotable(InSlotableClass);
	Slotables.Insert(SlotableInstance, InIndex);
	InitializeSlotable(SlotableInstance, Origin);
	ConstituentCount += SlotableInstance->GetConstituents().Num();
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
	CallBindedOnAddSlotableDelegates(SlotableInstance);
	return SlotableInstance;
}

bool UInventory::Server_SwapSlotables(USlotable* SlotableA, USlotable* SlotableB)
{
	if (!HasAuthority())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called Server_SwapSlotable on UInventory class %s without authority."),
		       *GetClass()->GetName());
		return false;
	}
	if (!SlotableA || !SlotableB) return false;
	if (!Slotables.Contains(SlotableA) || !Slotables.Contains(SlotableB))
	{
		UE_LOG(LogSfCore, Error,
		       TEXT(
			       "Called Server_SwapSlotables on UInventory class %s with at least one target not in the called UInventory."
		       ), *GetClass()->GetName());
		return false;
	}
	Server_SwapSlotablesByIndex(Slotables.Find(SlotableA), Slotables.Find(SlotableA));
	return true;
}

void UInventory::Server_SwapSlotablesByIndex(const int32 InIndexA, const int32 InIndexB)
{
	if (!HasAuthority())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called Server_SwapSlotableByIndex on UInventory class %s without authority."),
		       *GetClass()->GetName());
		return;
	}
	if (bIsChangeLocked)
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called Server_SwapSlotablesByIndex on UInventory class %s set to be a change-locked inventory."),
		       *GetClass()->GetName());
		return;
	}
	if (InIndexA < 0 || InIndexA >= Slotables.Num() || InIndexB < 0 || InIndexB >= Slotables.Num())
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called Server_SwapSlotablesByIndex on UInventory class %s with an index out of range."),
		       *GetClass()->GetName());
		return;
	}
	if (InIndexA == InIndexB)
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called Server_SwapSlotablesByIndex on UInventory class %s with identical indices."),
		       *GetClass()->GetName());
		return;
	}
	if (!Slotables[InIndexA] || !Slotables[InIndexB])
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called Server_SwapSlotablesByIndex on UInventory class %s with null USlotables."),
		       *GetClass()->GetName());
		return;
	}
	//We can just use last because all originating constituents in all constituents should be the same.
	UConstituent* OriginA = Slotables[InIndexA]->GetConstituents().Last()->GetOriginatingConstituent();
	UConstituent* OriginB = Slotables[InIndexB]->GetConstituents().Last()->GetOriginatingConstituent();
	DeinitializeSlotable(Slotables[InIndexA]);
	DeinitializeSlotable(Slotables[InIndexB]);
	USlotable* TempSlotablePtr = Slotables[InIndexA];
	Slotables[InIndexA] = Slotables[InIndexB];
	Slotables[InIndexB] = TempSlotablePtr;
	InitializeSlotable(Slotables[InIndexA], OriginB);
	InitializeSlotable(Slotables[InIndexB], OriginA);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
}

void UInventory::Server_TradeSlotablesBetweenInventories(USlotable* SlotableA, USlotable* SlotableB)
{
	//Cannot be made static as authority check needs to be made.
	if (!HasAuthority())
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called Server_TradeSlotablesBetweenInventories on UInventory class %s without authority."),
		       *GetClass()->GetName());
		return;
	}
	if (!SlotableA || !SlotableB)
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called Server_TradeSlotablesBetweenInventories on UInventory class %s with null USlotables."),
		       *GetClass()->GetName());
		return;
	}
	if (SlotableA == SlotableB)
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called Server_TradeSlotablesBetweenInventories on UInventory class %s with the same USlotable."),
		       *GetClass()->GetName());
		return;
	}
	UInventory* InventoryA = SlotableA->OwningInventory;
	UInventory* InventoryB = SlotableB->OwningInventory;
	//We can just use last because all originating constituents in all constituents should be the same.
	UConstituent* OriginA = SlotableA->GetConstituents().Last()->GetOriginatingConstituent();
	UConstituent* OriginB = SlotableB->GetConstituents().Last()->GetOriginatingConstituent();
	const int8 IndexA = InventoryA->GetSlotables().Find(SlotableA);
	const int8 IndexB = InventoryB->GetSlotables().Find(SlotableB);
	if (InventoryA->bIsChangeLocked || InventoryB->bIsChangeLocked)
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called Server_TradeSlotablesBetweenInventories on UInventory class %s with a locked UInventory."),
		       *GetClass()->GetName());
		return;
	}
	InventoryA->CallBindedOnRemoveSlotableDelegates(SlotableA);
	InventoryB->CallBindedOnRemoveSlotableDelegates(SlotableB);
	DeinitializeSlotable(SlotableA);
	DeinitializeSlotable(SlotableB);
	InventoryA->Slotables[IndexA] = DuplicateObject(SlotableB, SlotableA->GetTypedOuter<AActor>());
	InventoryB->Slotables[IndexB] = DuplicateObject(SlotableA, SlotableB->GetTypedOuter<AActor>());
	//We manually mark the object as garbage so its deletion can be replicated sooner to clients.
	SlotableA->Destroy();
	SlotableB->Destroy();
	InitializeSlotable(InventoryA->Slotables[IndexA], OriginB);
	InitializeSlotable(InventoryB->Slotables[IndexB], OriginA);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, InventoryA);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, InventoryB);
	InventoryA->CallBindedOnAddSlotableDelegates(InventoryA->Slotables[IndexA]);
	InventoryB->CallBindedOnAddSlotableDelegates(InventoryB->Slotables[IndexB]);
}

bool UInventory::Server_AddSharedCard(const TSubclassOf<UCardObject>& InCardClass, const float InCustomLifetime)
{
	//Id 0 is shared.
	return Server_AddOwnedCard(InCardClass, 0, InCustomLifetime);
}

bool UInventory::Server_RemoveSharedCard(const TSubclassOf<UCardObject>& InCardClass)
{
	//Id 0 is shared.
	return Server_RemoveOwnedCard(InCardClass, 0);
}

bool UInventory::Server_AddOwnedCard(const TSubclassOf<UCardObject>& InCardClass,
                                     const int32 InInOwnerConstituentInstanceId, const float InCustomLifetime)
{
	if (!HasAuthority())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called Server_AddOwnedCard on UInventory class %s without authority."),
		       *GetClass()->GetName());
		return false;
	}
	for (FCard Card : Cards)
	{
		//Check for duplicates.
		if (Card.Class == InCardClass && Card.OwnerConstituentInstanceId == InInOwnerConstituentInstanceId)
		{
			return false;
		}
	}
	if (InCustomLifetime != 0)
	{
		//References to FormCharacter and FormCore are needed because they provide timestamps for lifetime.
		if (IsFormCharacter())
		{
			Cards.Emplace(InCardClass, FCard::ECardType::UseCustomLifetimePredictedTimestamp,
			              InInOwnerConstituentInstanceId, GetFormCharacter(), nullptr, InCustomLifetime);
		}
		else
		{
			Cards.Emplace(InCardClass, FCard::ECardType::UseCustomLifetimeServerTimestamp,
			              InInOwnerConstituentInstanceId,
			              nullptr, OwningFormCore, InCustomLifetime);
		}
	}
	else
	{
		if (IsFormCharacter())
		{
			Cards.Emplace(InCardClass, FCard::ECardType::UseDefaultLifetimePredictedTimestamp,
			              InInOwnerConstituentInstanceId, GetFormCharacter());
		}
		else
		{
			Cards.Emplace(InCardClass, FCard::ECardType::UseCustomLifetimeServerTimestamp,
			              InInOwnerConstituentInstanceId,
			              nullptr, OwningFormCore);
		}
	}
	FCard& CardAdded = Cards.Last();
	if (IsFormCharacter())
	{
		GetFormCharacter()->bMovementSpeedNeedsRecalculation = true;
		//We set it to be not corrected and set a timeout so the client has a chance to synchronize before we start issuing corrections.
		CardAdded.bIsNotCorrected = true;
		CardAdded.ServerAwaitClientSyncTimeoutTimestamp = CardAdded.ServerAwaitClientSyncTimeoutDuration + GetWorld()->
			TimeSeconds;
		GetFormCharacter()->MarkCardsDirty();
	}
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Cards, this);
	CallBindedOnAddCardDelegates(CardAdded, false);
	return true;
}

bool UInventory::Server_RemoveOwnedCard(const TSubclassOf<UCardObject>& InCardClass,
                                        const int32 InOwnerConstituentInstanceId)
{
	if (!HasAuthority())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called Server_RemoveOwnedCard on UInventory class %s without authority."),
		       *GetClass()->GetName());
		return false;
	}
	for (FCard& Card : Cards)
	{
		if (Card.Class == InCardClass && Card.OwnerConstituentInstanceId == InOwnerConstituentInstanceId)
		{
			if (IsFormCharacter())
			{
				//Same concept as above.
				Card.bIsDisabledForDestroy = true;
				Card.ServerAwaitClientSyncTimeoutTimestamp = Card.ServerAwaitClientSyncTimeoutDuration + GetWorld()->
					TimeSeconds;
				GetFormCharacter()->MarkCardsDirty();
				MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Cards, this);
				return true;
			}
			//Otherwise remove it instantly.
			CallBindedOnRemoveCardDelegates(Card, false);
			Cards.Remove(Card);
			MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Cards, this);
			return true;
		}
	}
	if (IsFormCharacter())
	{
		GetFormCharacter()->bMovementSpeedNeedsRecalculation = true;
	}
	return false;
}

bool UInventory::Predicted_AddSharedCard(const TSubclassOf<UCardObject>& InCardClass, const float InCustomLifetime)
{
	return Predicted_AddOwnedCard(InCardClass, 0, InCustomLifetime);
}

bool UInventory::Predicted_RemoveSharedCard(const TSubclassOf<UCardObject>& InCardClass)
{
	return Predicted_RemoveOwnedCard(InCardClass, 0);
}

void UInventory::UpdateAndRunBufferedInputs(UConstituent* Constituent) const
{
	for (auto It = Constituent->BufferedInputs.CreateIterator(); It; ++It)
	{
		if (It->CheckConditionsMet(this, Constituent))
		{
			It->Delegate.ExecuteIfBound();
			It.RemoveCurrent();
		}
	}
}

bool UInventory::Predicted_AddOwnedCard(const TSubclassOf<UCardObject>& InCardClass,
                                        const int32 InOwnerConstituentInstanceId, const float InCustomLifetime)
{
	if (!GetOwner()) return false;
	if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		UE_LOG(LogSfCore, Error, TEXT("Called Predicted_AddOwnedCard on UInventory class %s on simulated proxy."),
		       *GetClass()->GetName());
		return false;
	}
	if (!IsFormCharacter())
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called Predicted_AddOwnedCard on UInventory class %s without a UFormCharacterComponent."),
		       *GetClass()->GetName());
		return false;
	}
	GetFormCharacter()->bMovementSpeedNeedsRecalculation = true;
	for (FCard Card : Cards)
	{
		//Check for duplicates.
		if (Card.Class == InCardClass && Card.OwnerConstituentInstanceId == InOwnerConstituentInstanceId)
		{
			return false;
		}
	}
	if (InCustomLifetime != 0)
	{
		//References to FormCharacter and FormCore are needed because they provide timestamps for lifetime.
		Cards.Emplace(InCardClass, FCard::ECardType::UseCustomLifetimePredictedTimestamp, InOwnerConstituentInstanceId,
		              GetFormCharacter(), nullptr, InCustomLifetime);
	}
	else
	{
		Cards.Emplace(InCardClass, FCard::ECardType::UseDefaultLifetimePredictedTimestamp, InOwnerConstituentInstanceId,
		              GetFormCharacter());
	}
	CallBindedOnAddCardDelegates(Cards.Last(), true);
	//We don't need to set correction pausing variables since we can start correcting instantly as this is predicted.
	if (HasAuthority())
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Cards, this);
	}
	else
	{
		ClientCheckAndUpdateCardObjects();
	}
	//Check buffered inputs.
	for (const USlotable* Slotable : Slotables)
	{
		for (UConstituent* Constituent : Slotable->GetConstituents())
		{
			UpdateAndRunBufferedInputs(Constituent);
		}
	}
	return true;
}

bool UInventory::Predicted_RemoveOwnedCard(const TSubclassOf<UCardObject>& InCardClass,
                                           const int32 InOwnerConstituentInstanceId)
{
	if (!GetOwner()) return false;
	if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		UE_LOG(LogSfCore, Error, TEXT("Called Predicted_RemoveOwnedCard on UInventory class %s on simulated proxy."),
		       *GetClass()->GetName());
		return false;
	}
	if (!IsFormCharacter())
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Called Predicted_RemoveOwnedCard on UInventory class %s without a UFormCharacterComponent."),
		       *GetClass()->GetName());
		return false;
	}
	GetFormCharacter()->bMovementSpeedNeedsRecalculation = true;
	for (FCard& Card : Cards)
	{
		if (Card.Class == InCardClass && Card.OwnerConstituentInstanceId == InOwnerConstituentInstanceId)
		{
			//We always instantly destroy as we should be able to sync instantly.
			CallBindedOnRemoveCardDelegates(Card, true);
			Cards.Remove(Card);
			if (HasAuthority())
			{
				MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Cards, this);
			}
			else
			{
				ClientCheckAndUpdateCardObjects();
			}
			//Check buffered inputs.
			for (USlotable* Slotable : Slotables)
			{
				for (UConstituent* Constituent : Slotable->GetConstituents())
				{
					UpdateAndRunBufferedInputs(Constituent);
				}
			}
			return true;
		}
	}
	return false;
}

void UInventory::AutonomousInitialize()
{
	bIsOnFormCharacter = IsFormCharacter();
	if (bIsOnFormCharacter)
	{
		//Translate InputActions to indices so we don't have to search when applying inputs.
		OrderedInputBindingIndices.Reserve(OrderedInputBindings.Num());
		OrderedLastInputState.Reserve(OrderedInputBindings.Num());
		for (UInputAction* Input : OrderedInputBindings)
		{
			if (Input == nullptr)
			{
				//We use INDEX_NONE to represent no input.
				OrderedInputBindingIndices.Add(INDEX_NONE);
			}
			else
			{
				const uint8 Index = GetFormCharacter()->InputActionRegistry.Find(Input);
				if (Index == INDEX_NONE)
				{
					UE_LOG(LogSfCore, Error,
					       TEXT(
						       "Found input on UInventory class %s that is unregistered with the UFormCharacterComponent. Skipping."
					       ), *GetClass()->GetName());
					continue;
				}
				OrderedInputBindingIndices.Add(Index);
			}
			OrderedLastInputState.Add(false);
		}
	}
	Autonomous_Initialize();
	bInitialized = true;
}

void UInventory::ServerInitialize()
{
	bIsOnFormCharacter = IsFormCharacter();
	if (bIsOnFormCharacter)
	{
		//Translate InputActions to indices so we don't have to search when applying inputs.
		OrderedInputBindingIndices.Reserve(OrderedInputBindings.Num());
		for (UInputAction* Input : OrderedInputBindings)
		{
			OrderedInputBindingIndices.Add(GetFormCharacter()->InputActionRegistry.Find(Input));
			OrderedLastInputState.Add(false);
		}
	}
	if (Capacity > 127)
	{
		UE_LOG(LogSfCore, Error,
						   TEXT(
							   "UInventory class %s has an inventory capacity of over 127."
						   ), *GetClass()->GetName());
		Capacity = 127;
	}
	Slotables.Reserve(InitialOrderedSlotableClasses.Num());
	for (const TSubclassOf<USlotable>& SlotableClass : InitialOrderedSlotableClasses)
	{
		USlotable* SlotableInstance = CreateUninitializedSlotable(SlotableClass);
		Slotables.Add(SlotableInstance);
		InitializeSlotable(SlotableInstance, nullptr);
	}
	if (TArrayCheckDuplicate(InitialSharedCardClassesInfiniteLifetime, [](const TSubclassOf<UCardObject>& A, const TSubclassOf<UCardObject>& B){return A == B;}))
	{
		UE_LOG(LogSfCore, Error,
						   TEXT(
							   "UInventory class %s has duplicate initial shared cards. Handling duplicates."
						   ), *GetClass()->GetName());
		auto ElementGroups = TArrayGroupEquivalentElements(InitialSharedCardClassesInfiniteLifetime);
		InitialSharedCardClassesInfiniteLifetime.Empty();
		InitialSharedCardClassesInfiniteLifetime.Reserve(ElementGroups.Num());
		for (auto It = ElementGroups.CreateIterator(); It; ++It)
		{
			InitialSharedCardClassesInfiniteLifetime.Add(It.Key());
		} 
	}
	Cards.Reserve(InitialSharedCardClassesInfiniteLifetime.Num());
	for (const TSubclassOf<UCardObject> CardClass : InitialSharedCardClassesInfiniteLifetime)
	{
		Cards.Emplace(CardClass, FCard::ECardType::DoNotUseLifetime);
	}
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Cards, this);
	ClientAutonomousInitialize(OwningFormCore);
	Server_Initialize();
	bInitialized = true;
}

void UInventory::AutonomousDeinitialize()
{
	Autonomous_Deinitialize();
	bInitialized = false;
}

void UInventory::ServerDeinitialize()
{
	Server_Deinitialize();
	ClientAutonomousDeinitialize();
	bInitialized = false;
}

void UInventory::AssignConstituentInstanceId(UConstituent* Constituent)
{
	//If less than max value - 1 we can keep adding.
	//0 is intentionally left unused.
	if (LastAssignedConstituentId < 255)
	{
		Constituent->InstanceId = LastAssignedConstituentId + 1;
		LastAssignedConstituentId++;
	}
	else
	{
		//If max value reached we reassign all to recycle values.
		ReassignAllConstituentInstanceIds();
	}
}

void UInventory::ReassignAllConstituentInstanceIds()
{
	LastAssignedConstituentId = 0;
	bool bOverflowing = false;
	for (const USlotable* Slotable : Slotables)
	{
		for (UConstituent* Constituent : Slotable->GetConstituents())
		{
			AssignConstituentInstanceId(Constituent);
			//Sanity check so we don't recursively call.
			//This shouldn't happen because there are checks when adding a slotable.
			if (LastAssignedConstituentId > 254)
			{
				UE_LOG(LogSfCore, Error,
						   TEXT(
							   "UInventory class %s has exceed the number of max UConstituents. Removing overflow."
						   ), *GetClass()->GetName());
				bOverflowing = true;
				break;
			}
		}
		if (bOverflowing) break;
	}
	//If we're overflowing we remove as many slotables as necessary to go back under the constituent max count.
	if (bOverflowing)
	{
		while (ConstituentCount > 254)
		{
			Server_RemoveSlotableByIndex(255, true);
		}
		ReassignAllConstituentInstanceIds();
	}
}

void UInventory::RemoveCardsOfOwner(const int32 InOwnerConstituentInstanceId)
{
	for (int16 i = Cards.Num() - 1; i >= 0; i--)
	{
		if (Cards[i].OwnerConstituentInstanceId == InOwnerConstituentInstanceId)
		{
			Server_RemoveOwnedCard(Cards[i].Class,Cards[i].OwnerConstituentInstanceId);
		}
	}
}

bool UInventory::IsDynamicLength() const
{
	return bIsDynamic;
}

TArray<const FCard*> UInventory::GetCardsOfClass(const TSubclassOf<UCardObject>& InClass) const
{
	TArray<const FCard*> Result;
	for (const FCard& Card : Cards)
	{
		if (Card.Class == InClass)
		{
			Result.Add(&Card);
		}
	}
	return Result;
}

void UInventory::Server_BindOnAddSlotable(const TSubclassOf<USlotable>& InSlotableClass,
	const FOnAddSlotable& EventToBind)
{
	if (!InSlotableClass.Get())
	{
		UE_LOG(LogSfCore, Error, TEXT("Server_BindOnAddSlotable called with empty TSubclassOf in UInventory class %s."), *GetClass()->GetName());
		return;
	}
	//Add to existing pair if possible, otherwise add pair.
	for (TPair<TSubclassOf<USlotable>, TSet<FOnAddSlotable>>& Pair : BindedOnAddSlotableDelegates)
	{
		if (Pair.Key == InSlotableClass)
		{
			Pair.Value.Add(EventToBind);
			return;
		}
	}
	BindedOnAddSlotableDelegates.Emplace(InSlotableClass, TSet{ EventToBind });
}

void UInventory::Server_BindOnRemoveSlotable(const TSubclassOf<USlotable>& InSlotableClass,
	const FOnRemoveSlotable& EventToBind)
{
	if (!InSlotableClass.Get())
	{
		UE_LOG(LogSfCore, Error, TEXT("Server_BindOnRemoveSlotable called with empty TSubclassOf in UInventory class %s."), *GetClass()->GetName());
		return;
	}
	//Add to existing pair if possible, otherwise add pair.
	for (TPair<TSubclassOf<USlotable>, TSet<FOnRemoveSlotable>>& Pair : BindedOnRemoveSlotableDelegates)
	{
		if (Pair.Key == InSlotableClass)
		{
			Pair.Value.Add(EventToBind);
			return;
		}
	}
	BindedOnRemoveSlotableDelegates.Emplace(InSlotableClass, TSet{ EventToBind });
}

void UInventory::BindOnAddCard(const TSubclassOf<UCardObject>& InCardClass, const FOnAddCard& EventToBind)
{
	if (!InCardClass.Get())
	{
		UE_LOG(LogSfCore, Error, TEXT("BindOnAddCard called with empty TSubclassOf in UInventory class %s."), *GetClass()->GetName());
		return;
	}
	//Add to existing pair if possible, otherwise add pair.
	for (TPair<TSubclassOf<UCardObject>, TSet<FOnAddCard>>& Pair : BindedOnAddCardDelegates)
	{
		if (Pair.Key == InCardClass)
		{
			Pair.Value.Add(EventToBind);
			return;
		}
	}
	BindedOnAddCardDelegates.Emplace(InCardClass, TSet{ EventToBind });
}

void UInventory::BindOnRemoveCard(const TSubclassOf<UCardObject>& InCardClass, const FOnRemoveCard& EventToBind)
{
	if (!InCardClass.Get())
	{
		UE_LOG(LogSfCore, Error, TEXT("BindOnRemoveCard called with empty TSubclassOf in UInventory class %s."), *GetClass()->GetName());
		return;
	}
	//Add to existing pair if possible, otherwise add pair.
	for (TPair<TSubclassOf<UCardObject>, TSet<FOnRemoveCard>>& Pair : BindedOnRemoveCardDelegates)
	{
		if (Pair.Key == InCardClass)
		{
			Pair.Value.Add(EventToBind);
			return;
		}
	}
	BindedOnRemoveCardDelegates.Emplace(InCardClass, TSet{ EventToBind });
}

void UInventory::CallBindedOnAddSlotableDelegates(USlotable* Slotable)
{
	for (TPair<TSubclassOf<USlotable>, TSet<FOnAddSlotable>>& Pair : BindedOnAddSlotableDelegates)
	{
		//Always call if it is USlotable.
		if (Pair.Key.Get() == USlotable::StaticClass())
		{
			for (FOnAddSlotable& Delegate : Pair.Value)
			{
				Delegate.ExecuteIfBound(Slotable);
			}
			continue;
		}
		//Otherwise call if it is the class of the added slotable.
		if (Pair.Key == Slotable->GetClass())
		{
			for (FOnAddSlotable& Delegate : Pair.Value)
			{
				Delegate.ExecuteIfBound(Slotable);
			}
		}
	}
}

void UInventory::CallBindedOnRemoveSlotableDelegates(USlotable* Slotable)
{
	for (TPair<TSubclassOf<USlotable>, TSet<FOnRemoveSlotable>>& Pair : BindedOnRemoveSlotableDelegates)
	{
		//Always call if it is USlotable.
		if (Pair.Key.Get() == USlotable::StaticClass())
		{
			for (FOnRemoveSlotable& Delegate : Pair.Value)
			{
				Delegate.ExecuteIfBound(Slotable);
			}
			continue;
		}
		//Otherwise call if it is the class of the removed slotable.
		if (Pair.Key == Slotable->GetClass())
		{
			for (FOnRemoveSlotable& Delegate : Pair.Value)
			{
				Delegate.ExecuteIfBound(Slotable);
			}
		}
	}
}

void UInventory::CallBindedOnAddCardDelegates(FCard& Card, const bool bInIsPredictableContext)
{
	for (TPair<TSubclassOf<UCardObject>, TSet<FOnAddCard>>& Pair : BindedOnAddCardDelegates)
	{
		//Always call if it is UCardObject.
		if (Pair.Key.Get() == UCardObject::StaticClass())
		{
			for (FOnAddCard& Delegate : Pair.Value)
			{
				Delegate.ExecuteIfBound(Card, bInIsPredictableContext);
			}
			continue;
		}
		//Otherwise call if it is the class of the added card.
		if (Pair.Key == Card.Class)
		{
			for (FOnAddCard& Delegate : Pair.Value)
			{
				Delegate.ExecuteIfBound(Card, bInIsPredictableContext);
			}
		}
	}
}

void UInventory::CallBindedOnRemoveCardDelegates(FCard& Card, const bool bInIsPredictableContext)
{
	for (TPair<TSubclassOf<UCardObject>, TSet<FOnRemoveCard>>& Pair : BindedOnRemoveCardDelegates)
	{
		//Always call if it is UCardObject.
		if (Pair.Key.Get() == UCardObject::StaticClass())
		{
			for (FOnRemoveCard& Delegate : Pair.Value)
			{
				Delegate.ExecuteIfBound(Card, bInIsPredictableContext);
			}
			continue;
		}
		//Otherwise call if it is the class of the removed card.
		if (Pair.Key == Card.Class)
		{
			for (FOnRemoveCard& Delegate : Pair.Value)
			{
				Delegate.ExecuteIfBound(Card, bInIsPredictableContext);
			}
		}
	}
}

//Must be called after the slotable has been placed in an inventory.
void UInventory::InitializeSlotable(USlotable* Slotable, UConstituent* Origin)
{
	if (!GetOwner()) return;
	GetOwner()->AddReplicatedSubObject(Slotable);
	Slotable->OwningInventory = this;
	MARK_PROPERTY_DIRTY_FROM_NAME(USlotable, OwningInventory, Slotable);
	for (UConstituent* Constituent : Slotable->GetConstituents())
	{
		Constituent->OriginatingConstituent = Origin;
		MARK_PROPERTY_DIRTY_FROM_NAME(UConstituent, OriginatingConstituent, Constituent);
	}
	Slotable->ServerInitialize();
}

//Must be called before the slotable is removed from an inventory.
void UInventory::DeinitializeSlotable(USlotable* Slotable)
{
	if (!GetOwner()) return;
	Slotable->ServerDeinitialize();
	Slotable->OwningInventory = nullptr;
	MARK_PROPERTY_DIRTY_FROM_NAME(USlotable, OwningInventory, Slotable);
	GetOwner()->RemoveReplicatedSubObject(Slotable);
}

void UInventory::OnRep_Slotables()
{
	//Register and deregister subobjects on client.
	for (USlotable* ReplicatedSlotable : Slotables)
	{
		if (!ReplicatedSlotable) continue;
		if (!ClientSubObjectListRegisteredSlotables.Contains(ReplicatedSlotable) && GetOwner())
		{
			GetOwner()->AddReplicatedSubObject(ReplicatedSlotable);
			ClientSubObjectListRegisteredSlotables.Add(ReplicatedSlotable);
		}
	}
	for (int16 i = ClientSubObjectListRegisteredSlotables.Num() - 1; i >= 0; i--)
	{
		if (!Slotables.Contains(ClientSubObjectListRegisteredSlotables[i]) && GetOwner())
		{
			GetOwner()->RemoveReplicatedSubObject(ClientSubObjectListRegisteredSlotables[i]);
			ClientSubObjectListRegisteredSlotables.RemoveAt(i, 1, false);
		}
	}
	ClientSubObjectListRegisteredSlotables.Shrink();
}

void UInventory::ClientAutonomousInitialize_Implementation(UFormCoreComponent* InOwningFormCore)
{
	OwningFormCore = InOwningFormCore;
	AutonomousInitialize();
}

void UInventory::ClientAutonomousDeinitialize_Implementation()
{
	AutonomousDeinitialize();
}

float UInventory::GetCardLifetime(const TSubclassOf<UCardObject>& InCardClass, const int32 InOwnerConstituentInstanceId)
{
	float Value = 0;
	for (const FCard& Card : Cards)
	{
		if (Card.bIsDisabledForDestroy) continue;
		if (Card.Class != InCardClass || Card.OwnerConstituentInstanceId != InOwnerConstituentInstanceId) continue;
		if (Card.LifetimeEndTimestamp <= 0) return 0;
		if (Card.bUsingPredictedTimestamp)
		{
			Value = GetFormCharacter()->CalculateTimeUntilPredictedTimestamp(Card.LifetimeEndTimestamp);
		}
		else
		{
			//Same function can be used for both server and client.
			Value = OwningFormCore->CalculateTimeUntilServerTimestamp(Card.LifetimeEndTimestamp);
		}
		break;
	}
	return Value;
}
