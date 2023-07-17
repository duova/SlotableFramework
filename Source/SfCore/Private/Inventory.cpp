// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory.h"

#include "Card.h"
#include "CardObject.h"
#include "FormCharacterComponent.h"
#include "FormCoreComponent.h"
#include "Slotable.h"
#include "Net/UnrealNetwork.h"

UInventory::UInventory()
{
	if (!GetOwner()) return;
	//We can directly init here because when the constructor is called, the object should have a reference to its outer.
	ClientInitialize();
}

void UInventory::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(UInventory, Slotables, DefaultParams);
	FDoRepLifetimeParams CardParams;
	CardParams.bIsPushBased = true;
	CardParams.Condition = COND_None;
	//We handle owner replication purely with the FormCharacterComponent if it is available.
	if (bInitialized)
	{
		if (GetOwner() && GetOwner()->FindComponentByClass(UFormCharacterComponent::StaticClass()))
		{
			CardParams.Condition = COND_SkipOwner;
		}
	}
	DOREPLIFETIME_WITH_PARAMS_FAST(UInventory, Cards, CardParams)
}

void UInventory::AuthorityTick(float DeltaTime)
{
	//We remove server timestamp cards with ended lifetimes only on the server.
	//This is synchronize to clients through the FormCharacter and normal replication depending on the role of the client.
	TArray<const FCard&> ToRemove;
	for (const FCard& Card : Cards)
	{
		if (!Card.bUsingPredictedTimestamp && OwningFormCore->CalculateTimeUntilServerTimestamp(Card.LifetimeEndTimestamp) < 0)
		{
			ToRemove.Emplace(Card);
		}
	}
	for (const FCard& CardToRemove : ToRemove)
	{
		Server_RemoveOwnedCard(CardToRemove.Class, CardToRemove.OwnerConstituentInstanceId);
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
			Slotables.RemoveAt(0);
			MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
		}
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
			ClientCardObjects.RemoveAt(0);
		}
		//We empty the list so the cards do not get created again.
		Cards.Empty();
		ClientDeinitialize();
	}
}

USlotable* UInventory::CreateUninitializedSlotable(const TSubclassOf<USlotable>& SlotableClass) const
{
	ErrorIfNoAuthority();
	if (!GetOwner()) return nullptr;
	if (!SlotableClass || SlotableClass->HasAnyClassFlags(CLASS_Abstract)) return nullptr;
	USlotable* SlotableInstance = NewObject<USlotable>(GetOwner(), SlotableClass);
	checkf(SlotableInstance, TEXT("Failed to create slotable."));
	return SlotableInstance;
}

UCardObject* UInventory::CreateUninitializedCardObject(const TSubclassOf<UCardObject>& CardClass) const
{
	if (!GetOwner()) return nullptr;
	if (!CardClass || CardClass->HasAnyClassFlags(CLASS_Abstract)) return nullptr;
	UCardObject* CardInstance = NewObject<UCardObject>(GetOwner(), CardClass);
	checkf(CardInstance, TEXT("Failed to create card."));
	return CardInstance;
}

void UInventory::ClientCheckAndUpdateCardObjects()
{
	TArray<UCardObject*> ToRemove;
	//Try to remove card objects that aren't on the cards list.
	for (UCardObject* CardObject : ClientCardObjects)
	{
		bool bHasCard = false;
		for (FCard Card : Cards)
		{
			if (Card.bIsDisabledForDestroy) continue;
			if (Card.Class == CardObject->GetClass() && Card.OwnerConstituentInstanceId == CardObject->
				OwnerConstituentInstanceId)
			{
				bHasCard = true;
				break;
			}
		}
		if (!bHasCard)
		{
			ToRemove.Add(CardObject);
		}
	}
	for (UCardObject* InvalidCard : ToRemove)
	{
		InvalidCard->Deinitialize();
		ClientCardObjects.Remove(InvalidCard);
	}
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
		TSubclassOf<UCardObject> CardClass{Card.Class};
		UCardObject* CardInstance = CreateUninitializedCardObject(CardClass);
		ClientCardObjects.Add(CardInstance);
		CardInstance->OwningInventory = this;
		CardInstance->OwnerConstituentInstanceId = Card.OwnerConstituentInstanceId;
		CardInstance->Initialize();
	}
}

TArray<USlotable*> UInventory::GetSlotables()
{
	TArray<USlotable*> SlotablesCopy;
	SlotablesCopy.Reserve(Slotables.Num());
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
	for (USlotable* Slotable : Slotables)
	{
		if (Slotable->GetClass() == SlotableClass)
		{
			Value = true;
			break;
		}
	}
	return Value;
}

uint8 UInventory::SlotableCount(const TSubclassOf<USlotable>& SlotableClass)
{
	uint8 Value = 0;
	for (const USlotable* Slotable : Slotables)
	{
		if (Slotable->GetClass() == SlotableClass) Value++;
	}
	return Value;
}

bool UInventory::HasSharedCard(const TSubclassOf<UCardObject>& CardClass)
{
	bool Value = false;
	for (FCard Card : Cards)
	{
		//If disabled we don't count it as existing.
		if (Card.bIsDisabledForDestroy) continue;
		if (Card.Class == CardClass && Card.OwnerConstituentInstanceId == 0)
		{
			Value = true;
			break;
		}
	}
	return Value;
}

bool UInventory::HasOwnedCard(const TSubclassOf<UCardObject>& CardClass, const uint8 InOwnerConstituentInstanceId)
{
	bool Value = false;
	for (FCard Card : Cards)
	{
		//If disabled we don't count it as existing.
		if (Card.bIsDisabledForDestroy) continue;
		if (Card.Class == CardClass && Card.OwnerConstituentInstanceId == InOwnerConstituentInstanceId)
		{
			Value = true;
			break;
		}
	}
	return Value;
}

float UInventory::GetSharedCardLifetime(const TSubclassOf<UCardObject>& CardClass)
{
	return GetCardLifetime(CardClass, 0);
}

float UInventory::GetOwnedCardLifetime(const TSubclassOf<UCardObject>& CardClass,
                                       const uint8 InOwnerConstituentInstanceId)
{
	return GetCardLifetime(CardClass, InOwnerConstituentInstanceId);
}

TArray<UCardObject*> UInventory::GetCardObjects(const TSubclassOf<UCardObject>& CardClass)
{
	TArray<UCardObject*> Objects;
	if (HasAuthority()) return Objects;
	for (UCardObject* Card : ClientCardObjects)
	{
		if (Card->GetClass() == CardClass) Objects.Add(Card);
	}
	return Objects;
}

USlotable* UInventory::Server_AddSlotable(const TSubclassOf<USlotable>& SlotableClass, UConstituent* Origin)
{
	ErrorIfNoAuthority();
	checkf(bIsDynamic, TEXT("Attempted to add slotable to static inventory."));
	checkf(!bIsChangeLocked, TEXT("Attempted to add slotable to change locked inventory."));
	checkf(Slotables.Num() < Capacity, TEXT("Attempted to add slotable to a full inventory."));
	if (ConstituentCount > 254 - SlotableClass.GetDefaultObject()->InitialConstituentClasses.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("Max constituent count in inventory reached. Slotable was not added."))
	}
	USlotable* SlotableInstance = CreateUninitializedSlotable(SlotableClass);
	Slotables.Add(SlotableInstance);
	SlotableInstance->OwningInventory = this;
	InitializeSlotable(SlotableInstance, Origin);
	ConstituentCount += SlotableInstance->GetConstituents().Num();
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
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
			ConstituentCount -= Slotable->GetConstituents().Num();
			DeinitializeSlotable(Slotable);
			//We manually mark the object as garbage so its deletion can be replicated sooner to clients.
			Slotable->Destroy();
		}
		Slotables.RemoveAt(Index);
	}
	else
	{
		Server_SetSlotable(EmptySlotableClass, Index, false, nullptr);
	}
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
}

USlotable* UInventory::Server_SetSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index,
                                          const bool bSlotMustBeNullOrEmpty, UConstituent* Origin)
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
	InitializeSlotable(SlotableInstance, Origin);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
	return SlotableInstance;
}

USlotable* UInventory::Server_InsertSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index, UConstituent* Origin)
{
	ErrorIfNoAuthority();
	checkf(bIsDynamic, TEXT("Attempted to insert slotable into a static inventory."));
	checkf(!bIsChangeLocked, TEXT("Attempted to insert slotable into a change locked inventory."));
	checkf(Index < 0 || Index >= Slotables.Num(), TEXT("Attempted to insert slotable with an index out of range."));
	USlotable* SlotableInstance = CreateUninitializedSlotable(SlotableClass);
	Slotables.Insert(SlotableInstance, Index);
	InitializeSlotable(SlotableInstance, Origin);
	ConstituentCount += SlotableInstance->GetConstituents().Num();
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
	//We can just use last because all originating constituents in all constituents should be the same.
	UConstituent* OriginA = Slotables[IndexA]->GetConstituents().Last()->GetOriginatingConstituent();
	UConstituent* OriginB = Slotables[IndexB]->GetConstituents().Last()->GetOriginatingConstituent();
	DeinitializeSlotable(Slotables[IndexA]);
	DeinitializeSlotable(Slotables[IndexB]);
	USlotable* TempSlotablePtr = Slotables[IndexA];
	Slotables[IndexA] = Slotables[IndexB];
	Slotables[IndexB] = TempSlotablePtr;
	InitializeSlotable(Slotables[IndexA], OriginB);
	InitializeSlotable(Slotables[IndexB], OriginA);
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
	//We can just use last because all originating constituents in all constituents should be the same.
	UConstituent* OriginA = SlotableA->GetConstituents().Last()->GetOriginatingConstituent();
	UConstituent* OriginB = SlotableB->GetConstituents().Last()->GetOriginatingConstituent();
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
	InitializeSlotable(InventoryA->Slotables[IndexA], OriginB);
	InitializeSlotable(InventoryB->Slotables[IndexB], OriginA);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, InventoryA);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, InventoryB);
}

bool UInventory::Server_AddSharedCard(const TSubclassOf<UCardObject>& CardClass, const float CustomLifetime)
{
	//Id 0 is shared.
	return Server_AddOwnedCard(CardClass, 0, CustomLifetime);
}

bool UInventory::Server_RemoveSharedCard(const TSubclassOf<UCardObject>& CardClass)
{
	//Id 0 is shared.
	return Server_RemoveOwnedCard(CardClass, 0);
}

bool UInventory::Server_AddOwnedCard(const TSubclassOf<UCardObject>& CardClass,
                                     const uint8 InOwnerConstituentInstanceId, const float CustomLifetime)
{
	ErrorIfNoAuthority();
	for (FCard Card : Cards)
	{
		//Check for duplicates.
		if (Card.Class == CardClass && Card.OwnerConstituentInstanceId == InOwnerConstituentInstanceId)
		{
			return false;
		}
	}
	if (CustomLifetime != 0)
	{
		//References to FormCharacter and FormCore are needed because they provide timestamps for lifetime.
		if (IsFormCharacter())
		{
			Cards.Emplace(CardClass, FCard::ECardType::UseCustomLifetimePredictedTimestamp,
			              InOwnerConstituentInstanceId, GetFormCharacter(), nullptr, CustomLifetime);
		}
		else
		{
			Cards.Emplace(CardClass, FCard::ECardType::UseCustomLifetimeServerTimestamp, InOwnerConstituentInstanceId,
			              nullptr, OwningFormCore, CustomLifetime);
		}
	}
	else
	{
		if (IsFormCharacter())
		{
			Cards.Emplace(CardClass, FCard::ECardType::UseDefaultLifetimePredictedTimestamp,
			              InOwnerConstituentInstanceId, GetFormCharacter());
		}
		else
		{
			Cards.Emplace(CardClass, FCard::ECardType::UseCustomLifetimeServerTimestamp, InOwnerConstituentInstanceId,
			              nullptr, OwningFormCore);
		}
	}
	if (IsFormCharacter())
	{
		FCard& CardAdded = Cards.Last();
		//We set it to be not corrected and set a timeout so the client has a chance to synchronize before we start issuing corrections.
		CardAdded.bIsNotCorrected = true;
		CardAdded.ServerAwaitClientSyncTimeoutTimestamp = CardAdded.ServerAwaitClientSyncTimeoutDuration + GetWorld()->TimeSeconds;
		GetFormCharacter()->MarkCardsDirty();
	}
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Cards, this);
	return true;
}

bool UInventory::Server_RemoveOwnedCard(const TSubclassOf<UCardObject>& CardClass,
                                        const uint8 InOwnerConstituentInstanceId)
{
	ErrorIfNoAuthority();
	int16 IndexToDestroy = INDEX_NONE;
	for (FCard Card : Cards)
	{
		if (Card.Class == CardClass && Card.OwnerConstituentInstanceId == InOwnerConstituentInstanceId)
		{
			if (IsFormCharacter())
			{
				//Same concept as above.
				Card.bIsDisabledForDestroy = true;
				Card.ServerAwaitClientSyncTimeoutTimestamp = Card.ServerAwaitClientSyncTimeoutDuration + GetWorld()->TimeSeconds;
				GetFormCharacter()->MarkCardsDirty();
				MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Cards, this);
				return true;
			}
			IndexToDestroy = Cards.Find(Card);
			break;
		}
	}
	if (IndexToDestroy != INDEX_NONE)
	{
		Cards.RemoveAt(IndexToDestroy);
		MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Cards, this);
		return true;
	}
	return false;
}

bool UInventory::Predicted_AddSharedCard(const TSubclassOf<UCardObject>& CardClass, float CustomLifetime)
{
	return Predicted_AddOwnedCard(CardClass, 0, CustomLifetime);
}

bool UInventory::Predicted_RemoveSharedCard(const TSubclassOf<UCardObject>& CardClass)
{
	return Predicted_RemoveOwnedCard(CardClass, 0);
}

bool UInventory::Predicted_AddOwnedCard(const TSubclassOf<UCardObject>& CardClass,
                                        const uint8 InOwnerConstituentInstanceId, float CustomLifetime)
{
	if (!GetOwner()) return false;
	checkf(GetOwner()->GetLocalRole() != ROLE_SimulatedProxy, TEXT("Called Predicted_ function on simulated proxy."))
	checkf(IsFormCharacter(), TEXT("Predicted_ function called by non-form character."));
	for (FCard Card : Cards)
	{
		//Check for duplicates.
		if (Card.Class == CardClass && Card.OwnerConstituentInstanceId == InOwnerConstituentInstanceId)
		{
			return false;
		}
	}
	if (CustomLifetime != 0)
	{
		//References to FormCharacter and FormCore are needed because they provide timestamps for lifetime.
		Cards.Emplace(CardClass, FCard::ECardType::UseCustomLifetimePredictedTimestamp, InOwnerConstituentInstanceId,
		              GetFormCharacter(), nullptr, CustomLifetime);
	}
	else
	{
		Cards.Emplace(CardClass, FCard::ECardType::UseDefaultLifetimePredictedTimestamp, InOwnerConstituentInstanceId,
		              GetFormCharacter());
	}
	//We don't need to set correction pausing variables since we can start correcting instantly as this is predicted.
	if (HasAuthority())
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Cards, this);
	}
	else
	{
		ClientCheckAndUpdateCardObjects();
	}
	return true;
}

bool UInventory::Predicted_RemoveOwnedCard(const TSubclassOf<UCardObject>& CardClass,
                                           const uint8 InOwnerConstituentInstanceId)
{
	if (!GetOwner()) return false;
	checkf(GetOwner()->GetLocalRole() != ROLE_SimulatedProxy, TEXT("Called Predicted_ function on simulated proxy."))
	checkf(IsFormCharacter(), TEXT("Predicted_ function called by non-form character."));
	int16 IndexToDestroy = INDEX_NONE;
	for (FCard Card : Cards)
	{
		if (Card.Class == CardClass && Card.OwnerConstituentInstanceId == InOwnerConstituentInstanceId)
		{
			//We always instantly destroy as we should be able to sync instantly.
			IndexToDestroy = Cards.Find(Card);
			break;
		}
	}
	if (IndexToDestroy != INDEX_NONE)
	{
		Cards.RemoveAt(IndexToDestroy);
		if (HasAuthority())
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Cards, this);
		}
		else
		{
			ClientCheckAndUpdateCardObjects();
		}
		return true;
	}
	return false;
}

void UInventory::ClientInitialize()
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
				checkf(Index != INDEX_NONE, TEXT("Inventory input is unregistered with the form character component."));
				OrderedInputBindingIndices.Add(Index);
			}
			OrderedLastInputState.Add(false);
		}
	}
	Client_Initialize();
	bInitialized = true;
}

void UInventory::ServerInitialize()
{
	bIsOnFormCharacter = IsFormCharacter();
	if (HasAuthority())
	{
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
		checkf(Capacity <= 128, TEXT("The capacity of an inventory may not exceed 128."))
		Slotables.Reserve(InitialOrderedSlotableClasses.Num());
		for (TSubclassOf<USlotable> SlotableClass : InitialOrderedSlotableClasses)
		{
			USlotable* SlotableInstance = CreateUninitializedSlotable(SlotableClass);
			Slotables.Add(SlotableInstance);
			InitializeSlotable(SlotableInstance, nullptr);
		}
		for (uint8 i = 0; i < InitialSharedCardClassesInfiniteLifetime.Num(); i++)
		{
			for (uint8 j = 0; j < InitialSharedCardClassesInfiniteLifetime.Num(); j++)
			{
				checkf(
					i == j || InitialSharedCardClassesInfiniteLifetime[i] != InitialSharedCardClassesInfiniteLifetime[j
					],
					TEXT("Each initial shared card in an inventory must be unique."));
			}
		}
		Cards.Reserve(InitialSharedCardClassesInfiniteLifetime.Num());
		for (const TSubclassOf<UCardObject> CardClass : InitialSharedCardClassesInfiniteLifetime)
		{
			Cards.Emplace(CardClass, FCard::ECardType::DoNotUseLifetime);
		}
		MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Slotables, this);
		MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, Cards, this);
	}
	Server_Initialize();
	bInitialized = true;
}

void UInventory::ClientDeinitialize()
{
	Client_Deinitialize();
	bInitialized = false;
}

void UInventory::ServerDeinitialize()
{
	Server_Deinitialize();
	bInitialized = false;
}

void UInventory::AssignConstituentInstanceId(UConstituent* Constituent)
{
	//If less than max value - 1 we can keep adding.
	//0 is intentionally left unused.
	if (LastAssignedConstituentId < 255 - 1)
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
	for (USlotable* Slotable : Slotables)
	{
		for (UConstituent* Constituent : Slotable->GetConstituents())
		{
			AssignConstituentInstanceId(Constituent);
			//Sanity check so we don't recursively call.
			checkf(LastAssignedConstituentId < 255 - 1, TEXT("Only 254 constituents may be in an inventory."));
		}
	}
}

void UInventory::RemoveCardsOfOwner(const uint8 OwnerConstituentInstanceId)
{
	TArray<TSubclassOf<UCardObject>> ToRemove;
	for (FCard Card : Cards)
	{
		if (Card.OwnerConstituentInstanceId == OwnerConstituentInstanceId)
		{
			ToRemove.Add(Card.Class);
		}
	}
	for (const TSubclassOf<UCardObject> CardClass : ToRemove)
	{
		Server_RemoveOwnedCard(CardClass, OwnerConstituentInstanceId);
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

float UInventory::GetCardLifetime(const TSubclassOf<UCardObject>& CardClass, const int32 InOwnerConstituentInstanceId)
{
	float Value = 0;
	for (FCard Card : Cards)
	{
		if (Card.bIsDisabledForDestroy) continue;
		if (Card.Class != CardClass || Card.OwnerConstituentInstanceId != InOwnerConstituentInstanceId) continue;
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
