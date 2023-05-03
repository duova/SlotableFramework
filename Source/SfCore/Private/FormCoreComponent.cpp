// Fill out your copyright notice in the Description page of Project Settings.


#include "SfCore/Public/FormCoreComponent.h"

UFormCoreComponent::UFormCoreComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bReplicateUsingRegisteredSubObjectList = true;
	SetIsReplicatedByDefault(true);
}


void UFormCoreComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetOwner()->HasAuthority()) return;
	for (TSubclassOf<UInventory> InventoryClass : DefaultInventoryClasses)
	{
		AddInventory(InventoryClass);
	}
}

void UFormCoreComponent::BeginDestroy()
{
	Super::BeginDestroy();
	if (!GetOwner()->HasAuthority()) return;
	for (int32 i = 0; i < Inventories.Num(); i++)
	{
		//We clear index 0 because the list shifts down.
		RemoveInventoryByIndex(0);
	}
}

void UFormCoreComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UFormCoreComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(UFormCoreComponent, Inventories, DefaultParams);
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

UInventory* UFormCoreComponent::AddInventory(const TSubclassOf<UInventory>& InventoryClass)
{
	if (!InventoryClass || InventoryClass->HasAnyClassFlags(CLASS_Abstract)) return nullptr;
	const AActor* Owner = GetOwner();
	checkf(Owner != nullptr, TEXT("Invalid owner."));
	checkf(Owner->HasAuthority(), TEXT("Called without authority."));
	UInventory* InventoryInstance = NewObject<UInventory>(GetOwner(), InventoryClass);
	checkf(InventoryInstance, TEXT("Failed to create inventory."));
	Inventories.Emplace(InventoryInstance);
	GetOwner()->AddReplicatedSubObject(InventoryInstance);
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, Inventories, this);
	return InventoryInstance;
}

void UFormCoreComponent::RemoveInventoryByIndex(const int32 Index)
{
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
	//We manually mark the object as garbage so its deletion can be replicated sooner to clients.
	Inventory->Destroy();
	Inventories.RemoveAt(Index);
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, Inventories, this);
}

bool UFormCoreComponent::RemoveInventory(UInventory* Inventory)
{
	//Authority checking done by RemoveInventoryByIndex().
	if (!Inventory) return false;
	const int32 Index = Inventories.Find(Inventory);
	if (Index == INDEX_NONE) return false;
	RemoveInventoryByIndex(Index);
	return true;
}
