// Fill out your copyright notice in the Description page of Project Settings.


#include "SfCore/Public/FormCoreComponent.h"

#include "CardObject.h"
#include "Constituent.h"
#include "Inventory.h"
#include "Slotable.h"
#include "FormCharacterComponent.h"
#include "SfHealthComponent.h"
#include "FormQueryComponent.h"
#include "FormStatComponent.h"
#include "Net/UnrealNetwork.h"

UFormCoreComponent::UFormCoreComponent()
{
	if (!GetOwner()) return;
	PrimaryComponentTick.bCanEverTick = true;
	bReplicateUsingRegisteredSubObjectList = true;
	SetIsReplicated(true);
}

const TArray<UClass*>& UFormCoreComponent::GetAllCardObjectClassesSortedByName()
{
	return AllCardObjectClassesSortedByName;
}

UFormQueryComponent* UFormCoreComponent::GetFormQuery() const
{
	return FormQuery;
}

USfHealthComponent* UFormCoreComponent::GetHealth() const
{
	return SfHealth;
}

UFormStatComponent* UFormCoreComponent::GetFormStat() const
{
	return FormStat;
}

bool UFormCoreComponent::ActivateTrigger(const FGameplayTag Trigger)
{
	for (const TPair<FGameplayTag, FTriggerDelegate>& Pair : Triggers)
	{
		if (Pair.Key == Trigger)
		{
			if (Pair.Value.IsBound())
			{
				Pair.Value.Broadcast();
			}
			return true;
		}
	}
	return false;
}

bool UFormCoreComponent::BindTrigger(const FGameplayTag Trigger, const FTriggerInputDelegate EventToBind)
{
	for (TPair<FGameplayTag, FTriggerDelegate>& Pair : Triggers)
	{
		if (Pair.Key == Trigger)
		{
			Pair.Value.Add(EventToBind);
			return true;
		}
	}
	return false;
}

bool UFormCoreComponent::HasTrigger(const FGameplayTag Trigger)
{
	for (const TPair<FGameplayTag, FTriggerDelegate>& Pair : Triggers)
	{
		if (Pair.Key == Trigger)
		{
			return true;
		}
	}
	return false;
}

void UFormCoreComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetOwner()) return;
	FormCharacter = Cast<UFormCharacterComponent>(GetOwner()->FindComponentByClass(UFormCharacterComponent::StaticClass()));
	FormQuery = Cast<UFormQueryComponent>(GetOwner()->FindComponentByClass(UFormQueryComponent::StaticClass()));
	SfHealth = Cast<USfHealthComponent>(GetOwner()->FindComponentByClass(USfHealthComponent::StaticClass()));
	FormStat = Cast<UFormStatComponent>(GetOwner()->FindComponentByClass(UFormStatComponent::StaticClass()));
	
	if (FormCharacter)
	{
		FormCharacter->SetupFormCharacter(this);
	}

	if (SfHealth)
	{
		SfHealth->SetupSfHealth(this);
	}

	if (FormQuery)
	{
		FormQuery->SetupFormQuery(this);
	}
	
	//This is used to narrow down the classes that need to be iterated through when serializing card classes with names.
	if (!CardObjectClassesFetched)
	{
		for(TObjectIterator<UClass> It; It; ++It)
		{
			if(It->IsChildOf(UCardObject::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract))
			{
				AllCardObjectClassesSortedByName.Add(*It);
			}
		}
		//We sort this in a deterministic order to index items.
		AllCardObjectClassesSortedByName.Sort([](const UClass& A, const UClass& B) { return A.GetFullName() > B.GetFullName(); });
		CardObjectClassesFetched = true;
	}

	if (GetOwner())
	{
		if (GetOwner()->HasAuthority())
		{
			for (TSubclassOf<UInventory> InventoryClass : DefaultInventoryClasses)
			{
				Server_AddInventory(InventoryClass);
			}

			for (const FGameplayTag& TriggerTag : TriggersToUse)
			{
				if (Triggers.Contains(TriggerTag))
				{
					UE_LOG(LogTemp, Warning, TEXT("Duplicated trigger."));
					continue;
				}
				Triggers.Add(TriggerTag, FTriggerDelegate());
			}
		}
	}

	FormCharacter->CalculateMovementSpeed();
}

void UFormCoreComponent::BeginDestroy()
{
	Super::BeginDestroy();
	if (!GetOwner()) return;
	if (!GetOwner()->HasAuthority()) return;
	for (int32 i = 0; i < Inventories.Num(); i++)
	{
		//We clear index 0 because the list shifts down.
		Server_RemoveInventoryByIndex(0);
	}
}

void UFormCoreComponent::OnRep_Inventories()
{
	//Register and deregister subobjects on client.
	for (UInventory* ReplicatedInventory : Inventories)
	{
		if (!ReplicatedInventory) continue;
		if (!ClientSubObjectListRegisteredInventories.Contains(ReplicatedInventory) && GetOwner())
		{
			GetOwner()->AddReplicatedSubObject(ReplicatedInventory);
			ClientSubObjectListRegisteredInventories.Add(ReplicatedInventory);
		}
	}
	TArray<UInventory*> ToRemove;
	for (UInventory* RegisteredInventory : ClientSubObjectListRegisteredInventories)
	{
		if (!Inventories.Contains(RegisteredInventory) && GetOwner())
		{
			GetOwner()->RemoveReplicatedSubObject(RegisteredInventory);
			ToRemove.Add(RegisteredInventory);
		}
	}
	for (UInventory* InventoryToRemove : ToRemove)
	{
		ClientSubObjectListRegisteredInventories.Remove(InventoryToRemove);
	}
}

void UFormCoreComponent::SetMovementStatsDirty() const
{
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, WalkSpeedStat, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, SwimSpeedStat, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, FlySpeedStat, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, AccelerationStat, this);
}

void UFormCoreComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GetOwner()->HasAuthority()) return;
	
	for (UInventory* Inventory : Inventories)
	{
		Inventory->AuthorityTick(DeltaTime);
	}
	
	if (GetOwner()->HasAuthority())
	{
		NonCompensatedServerWorldTime = GetWorld()->TimeSeconds;
	}
	else
	{
		NonCompensatedServerWorldTime += DeltaTime;
	}
	
	if (static_cast<int>(NonCompensatedServerWorldTime) % 2)
	{
		//We synchronize every 2 seconds.
		MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, NonCompensatedServerWorldTime, this);
	}

	if (!GetOwner()->HasAuthority()) return;
	LowFrequencyTickDeltaTime += DeltaTime;
	if (LowFrequencyTickDeltaTime >= 1.0 / LowFrequencyTicksPerSecond)
	{
		for (UConstituent* Constituent : ConstituentRegistry)
		{
			Constituent->Server_LowFrequencyTick(LowFrequencyTickDeltaTime);
		}
		LowFrequencyTickDeltaTime = 0;
	}
}

void UFormCoreComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormCoreComponent, Inventories, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormCoreComponent, Team, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormCoreComponent, NonCompensatedServerWorldTime, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormCoreComponent, WalkSpeedStat, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormCoreComponent, SwimSpeedStat, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormCoreComponent, FlySpeedStat, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormCoreComponent, AccelerationStat, DefaultParams);
}

TArray<UInventory*> UFormCoreComponent::GetInventories()
{
	TArray<UInventory*> InventoriesCopy;
	for (UInventory* Inventory : Inventories)
	{
		InventoriesCopy.Add(Inventory);
	}
	return InventoriesCopy;
}

FGameplayTag UFormCoreComponent::GetTeam()
{
	return Team;
}

void UFormCoreComponent::Server_SetTeam(const FGameplayTag InTeam)
{
	Team = InTeam;
}

UInventory* UFormCoreComponent::Server_AddInventory(const TSubclassOf<UInventory>& InventoryClass)
{
	if (!InventoryClass || InventoryClass->HasAnyClassFlags(CLASS_Abstract)) return nullptr;
	if (!GetOwner()) return nullptr;
	const AActor* Owner = GetOwner();
	checkf(Owner != nullptr, TEXT("Invalid owner."));
	checkf(Owner->HasAuthority(), TEXT("Called without authority."));
	UInventory* InventoryInstance = NewObject<UInventory>(GetOwner(), InventoryClass);
	checkf(InventoryInstance, TEXT("Failed to create inventory."));
	Inventories.Emplace(InventoryInstance);
	InventoryInstance->OwningFormCore = this;
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, OwningFormCore, InventoryInstance);
	GetOwner()->AddReplicatedSubObject(InventoryInstance);
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, Inventories, this);
	InventoryInstance->ServerInitialize();
	return InventoryInstance;
}

void UFormCoreComponent::Server_RemoveInventoryByIndex(const int32 Index)
{
	if (!GetOwner()) return;
	const AActor* Owner = GetOwner();
	checkf(Owner, TEXT("Invalid Owner."));
	checkf(Owner->HasAuthority(), TEXT("Called without Authority."));
	checkf(Index < 0 || Index >= Inventories.Num(), TEXT("Attempted to remove an inventory with an index out of range."));
	UInventory* Inventory = Inventories[Index];
	if (!Inventory)
	{
		Inventories.RemoveAt(Index);
		return;
	}
	GetOwner()->RemoveReplicatedSubObject(Inventory);
	Inventory->ServerDeinitialize();
	//We manually mark the object as garbage so its deletion can be replicated sooner to clients.
	Inventory->Destroy();
	Inventories.RemoveAt(Index);
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, Inventories, this);
}

bool UFormCoreComponent::Server_RemoveInventory(UInventory* Inventory)
{
	//Authority checking done by Server_RemoveInventoryByIndex().
	if (!Inventory) return false;
	const int32 Index = Inventories.Find(Inventory);
	if (Index == INDEX_NONE) return false;
	Server_RemoveInventoryByIndex(Index);
	return true;
}

void UFormCoreComponent::ClientSetToFirstPerson()
{
	bIsFirstPerson = true;
	//Refresh actions to use new one. (ie. use effects from the new perspective)
	for (UInventory* Inventory : Inventories)
	{
		for (USlotable* Slotable : Inventory->GetSlotables())
		{
			for (UConstituent* Constituent : Slotable->GetConstituents())
			{
				Constituent->OnRep_LastActionSet();
			}
		}
	}
}

void UFormCoreComponent::ClientSetToThirdPerson()
{
	bIsFirstPerson = false;
	//Refresh actions to use new one. (ie. use effects from the new perspective)
	for (UInventory* Inventory : Inventories)
	{
		for (USlotable* Slotable : Inventory->GetSlotables())
		{
			for (UConstituent* Constituent : Slotable->GetConstituents())
			{
				Constituent->OnRep_LastActionSet();
			}
		}
	}
}

bool UFormCoreComponent::IsFirstPerson()
{
	return bIsFirstPerson;
}

float UFormCoreComponent::GetNonCompensatedServerWorldTime() const
{
	return NonCompensatedServerWorldTime;
}

float UFormCoreComponent::CalculateFutureServerTimestamp(const float AdditionalTime) const
{
	return NonCompensatedServerWorldTime + AdditionalTime;
}

float UFormCoreComponent::CalculateTimeUntilServerTimestamp(const float Timestamp) const
{
	return NonCompensatedServerWorldTime - Timestamp;
}

float UFormCoreComponent::CalculateTimeSinceServerTimestamp(const float Timestamp) const
{
	return Timestamp - NonCompensatedServerWorldTime;
}

bool UFormCoreComponent::HasServerTimestampPassed(const float Timestamp) const
{
	return CalculateTimeUntilServerTimestamp(Timestamp) <= 0;
}
