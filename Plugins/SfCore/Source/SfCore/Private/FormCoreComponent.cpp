// Fill out your copyright notice in the Description page of Project Settings.


#include "SfCore/Public/FormCoreComponent.h"

#include "CardObject.h"
#include "Constituent.h"
#include "Inventory.h"
#include "FormCharacterComponent.h"
#include "SfHealthComponent.h"
#include "FormQueryComponent.h"
#include "FormResourceComponent.h"
#include "FormStatComponent.h"
#include "Net/UnrealNetwork.h"

UFormCoreComponent::UFormCoreComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bReplicateUsingRegisteredSubObjectList = true;
	SetIsReplicatedByDefault(true);
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

UFormResourceComponent* UFormCoreComponent::GetFormResource() const
{
	return FormResource;
}

bool UFormCoreComponent::Server_ActivateTrigger(const FGameplayTag Trigger)
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

bool UFormCoreComponent::Server_BindTrigger(const FGameplayTag Trigger, const FTriggerInputDelegate EventToBind)
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

bool UFormCoreComponent::Server_HasTrigger(const FGameplayTag Trigger)
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
	FormCharacter = Cast<UFormCharacterComponent>(
		GetOwner()->FindComponentByClass(UFormCharacterComponent::StaticClass()));
	FormQuery = Cast<UFormQueryComponent>(GetOwner()->FindComponentByClass(UFormQueryComponent::StaticClass()));
	SfHealth = Cast<USfHealthComponent>(GetOwner()->FindComponentByClass(USfHealthComponent::StaticClass()));
	FormStat = Cast<UFormStatComponent>(GetOwner()->FindComponentByClass(UFormStatComponent::StaticClass()));
	FormResource = Cast<UFormResourceComponent>(GetOwner()->FindComponentByClass(UFormResourceComponent::StaticClass()));
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, FormCharacter, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, FormQuery, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, SfHealth, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, FormStat, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, FormResource, this);

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

	if (FormResource)
	{
		FormResource->SetupFormResource(this);
	}

	if (FormStat)
	{
		FormStat->SetupFormStat();
	}

	if (SfHealth)
	{
		SfHealth->SecondarySetupSfHealth();
	}

	if (FormCharacter)
	{
		FormCharacter->SecondarySetupFormCharacter();
	}
	
	if (FormResource)
	{
		FormResource->SecondarySetupFormResource();
	}

	//This is used to narrow down the classes that need to be iterated through when serializing card classes with names.
	if (!CardObjectClassesFetched)
	{
		AllCardObjectClassesSortedByName = GetSubclassesOf(UCardObject::StaticClass());
		for (int64 i = AllCardObjectClassesSortedByName.Num() - 1; i >= 0; i--)
		{
			if (AllCardObjectClassesSortedByName[i]->HasAnyClassFlags(CLASS_Abstract) || AllCardObjectClassesSortedByName[i] == UCardObject::StaticClass())
			{
				AllCardObjectClassesSortedByName.RemoveAt(i, 1, false);
			}
		}
		AllCardObjectClassesSortedByName.Shrink();
		//We sort this in a deterministic order to index items.
		AllCardObjectClassesSortedByName.Sort([](const UClass& A, const UClass& B)
		{
			return A.GetPathName() > B.GetPathName();
		});
		CardObjectClassesFetched = true;
	}

	if (GetOwner()->HasAuthority())
	{
		Inventories.Reserve(DefaultInventoryClasses.Num());
		for (TSubclassOf<UInventory> InventoryClass : DefaultInventoryClasses)
		{
			if (!InventoryClass.Get())
			{
				UE_LOG(LogSfCore, Warning, TEXT("FormCoreComponent class %s has an empty inventory TSubclassOf."), *GetClass()->GetName());
				continue;
			}
			Server_AddInventory(InventoryClass);
		}

		Triggers.Reserve(TriggersToUse.Num());
		for (const FGameplayTag& TriggerTag : TriggersToUse)
		{
			if (Triggers.Contains(TriggerTag))
			{
				UE_LOG(LogSfCore, Warning, TEXT("Duplicated trigger on FormCoreComponent class %s."), *GetClass()->GetName());
				continue;
			}
			Triggers.Add(TriggerTag, FTriggerDelegate());
		}
	}

	if (FormCharacter)
	{
		FormCharacter->CalculateMovementSpeed();
	}
	
	CalculatedTimeBetweenLowFrequencyTicks = 1.0 / LowFrequencyTicksPerSecond;
}

void UFormCoreComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		for (int16 i = Inventories.Num() - 1; i >= 0; i--)
		{
			Server_RemoveInventoryByIndex(i);
		}
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
	for (auto It = ClientSubObjectListRegisteredInventories.CreateIterator(); It; ++It)
	{
		if (!Inventories.Contains(*It) && GetOwner())
		{
			GetOwner()->RemoveReplicatedSubObject(*It);
			It.RemoveCurrent();
		}
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
	if (!GetOwner()) return;

	if (bInputsRequireSetup && FormCharacter)
	{
		for (UInventory* Inventory : Inventories)
		{
			Inventory->SetupInputs(FormCharacter);
		}
		bInputsRequireSetup = false;
	}

	NonCompensatedServerFormTime += DeltaTime;

	if (NonCompensatedServerFormTime * 0.1 - FMath::Floor(NonCompensatedServerFormTime * 0.1) <= DeltaTime * 0.1)
	{
		//Remove empty delegate bindings for inventories every 10 seconds.
		for (UInventory* Inventory : Inventories)
		{
			Inventory->RemoveEmptyDelegateBindings();
		}
	}
	
	if (!GetOwner()->HasAuthority()) return;
	//Server only.

	for (UInventory* Inventory : Inventories)
	{
		Inventory->AuthorityTick(DeltaTime);
	}

	for (UConstituent* Constituent : ConstituentRegistry)
	{
		if (!Constituent->bLastActionSetPendingClientExecution) return;
		Constituent->PerformLastActionSetOnClients();
		Constituent->bLastActionSetPendingClientExecution = false;
	}

	if (NonCompensatedServerFormTime * 0.5 - FMath::Floor(NonCompensatedServerFormTime * 0.5) <= DeltaTime * 0.5)
	{
		//We synchronize every 2 seconds.
		MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, NonCompensatedServerFormTime, this);
	}
	
	LowFrequencyTickDeltaTime += DeltaTime;
	if (LowFrequencyTickDeltaTime > CalculatedTimeBetweenLowFrequencyTicks)
	{
		for (UConstituent* Constituent : ConstituentRegistry)
		{
			if (!Constituent->bEnableLowFreqTick) continue;
			Constituent->Server_LowFrequencyTick(LowFrequencyTickDeltaTime);
		}
		LowFrequencyTickDeltaTime -= CalculatedTimeBetweenLowFrequencyTicks;
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
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormCoreComponent, NonCompensatedServerFormTime, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormCoreComponent, WalkSpeedStat, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormCoreComponent, SwimSpeedStat, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormCoreComponent, FlySpeedStat, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormCoreComponent, AccelerationStat, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormCoreComponent, ConstituentRegistry, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormCoreComponent, FormCharacter, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormCoreComponent, FormQuery, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormCoreComponent, SfHealth, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormCoreComponent, FormStat, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormCoreComponent, FormResource, DefaultParams);
}

const TArray<UInventory*>& UFormCoreComponent::GetInventories()
{
	return Inventories;
}

FGameplayTag UFormCoreComponent::GetTeam()
{
	return Team;
}

void UFormCoreComponent::Server_SetTeam(const FGameplayTag InTeam)
{
	Team = InTeam;
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, Team, this);
}

UInventory* UFormCoreComponent::Server_AddInventory(const TSubclassOf<UInventory>& InInventoryClass)
{
	if (!InInventoryClass.Get() || InInventoryClass->HasAnyClassFlags(CLASS_Abstract)) return nullptr;
	if (!GetOwner()) return nullptr;
	const AActor* Owner = GetOwner();
	if (!Owner->HasAuthority())
	{
		UE_LOG(LogSfCore, Error, TEXT("Server_AddInventory called without authority on UFormCoreComponent class %s."), *GetClass()->GetName());
		return nullptr;
	}
	UInventory* InventoryInstance = NewObject<UInventory>(GetOwner(), InInventoryClass);
	if (!InventoryInstance)
	{
		UE_LOG(LogSfCore, Error, TEXT("Failed to create inventory on UFormCoreComponent class %s."), *GetClass()->GetName());
		return nullptr;
	}
	Inventories.Add(InventoryInstance);
	InventoryInstance->OwningFormCore = this;
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventory, OwningFormCore, InventoryInstance);
	GetOwner()->AddReplicatedSubObject(InventoryInstance);
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, Inventories, this);
	InventoryInstance->ServerInitialize();
	return InventoryInstance;
}

void UFormCoreComponent::Server_RemoveInventoryByIndex(const int32 InIndex)
{
	if (!GetOwner()) return;
	const AActor* Owner = GetOwner();
	if (!Owner->HasAuthority())
	{
		UE_LOG(LogSfCore, Error, TEXT("Server_RemoveInventoryByIndex called without authority on UFormCoreComponent class %s."), *GetClass()->GetName());
		return;
	}
	if (InIndex < 0 || InIndex >= Inventories.Num())
	{
		UE_LOG(LogSfCore, Error, TEXT("Attempted to remove an inventory with an index out of range on UFormCoreComponent class %s."), *GetClass()->GetName());
		return;
	}
	UInventory* Inventory = Inventories[InIndex];
	if (!Inventory)
	{
		Inventories.RemoveAt(InIndex);
		return;
	}
	GetOwner()->RemoveReplicatedSubObject(Inventory);
	Inventory->ServerDeinitialize();
	//We manually mark the object as garbage so its deletion can be replicated sooner to clients.
	Inventory->Destroy();
	Inventories.RemoveAt(InIndex);
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, Inventories, this);
}

bool UFormCoreComponent::Server_RemoveInventory(UInventory* Inventory)
{
	//Authority checking done by Server_RemoveInventoryByIndex().
	if (!Inventory) return false;
	const int16 Index = Inventories.Find(Inventory);
	if (Index == INDEX_NONE) return false;
	Server_RemoveInventoryByIndex(Index);
	return true;
}

void UFormCoreComponent::Client_SetToFirstPerson()
{
	InternalClientSetToFirstPerson();
}

void UFormCoreComponent::Client_SetToThirdPerson()
{
	InternalClientSetToThirdPerson();
}

void UFormCoreComponent::InternalClientSetToFirstPerson()
{
	if (GetOwner()->HasAuthority())
	{
		UE_LOG(LogSfCore, Error, TEXT("Server tried to call ClientSetToFirstPerson on UFormCoreComponent class %s."),
		       *GetClass()->GetName());
		return;
	}
	bIsFirstPerson = true;
	//Refresh actions to use new one. (ie. use effects from the new perspective)
	for (UConstituent* Constituent : ConstituentRegistry)
	{
		Constituent->InternalClientPerformActionSet();
	}
}

void UFormCoreComponent::InternalClientSetToThirdPerson()
{
	if (GetOwner()->HasAuthority())
	{
		UE_LOG(LogSfCore, Error, TEXT("Server tried to call ClientSetToFirstPerson on UFormCoreComponent class %s."),
		       *GetClass()->GetName());
		return;
	}
	bIsFirstPerson = false;
	//Refresh actions to use new one. (ie. use effects from the new perspective)
	for (UConstituent* Constituent : ConstituentRegistry)
	{
		Constituent->InternalClientPerformActionSet();
	}
}

bool UFormCoreComponent::IsFirstPerson()
{
	return bIsFirstPerson;
}

float UFormCoreComponent::GetNonCompensatedServerFormTime() const
{
	return NonCompensatedServerFormTime;
}

float UFormCoreComponent::CalculateFutureServerFormTimestamp(const float InAdditionalTime) const
{
	return NonCompensatedServerFormTime + InAdditionalTime;
}

float UFormCoreComponent::CalculateTimeUntilServerFormTimestamp(const float InTimestamp) const
{
	return InTimestamp - NonCompensatedServerFormTime;
}

float UFormCoreComponent::CalculateTimeSinceServerFormTimestamp(const float InTimestamp) const
{
	return NonCompensatedServerFormTime - InTimestamp;
}

bool UFormCoreComponent::HasServerFormTimestampPassed(const float InTimestamp) const
{
	return CalculateTimeUntilServerFormTimestamp(InTimestamp) <= 0;
}
