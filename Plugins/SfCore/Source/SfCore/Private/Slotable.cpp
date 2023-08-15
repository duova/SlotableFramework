// Fill out your copyright notice in the Description page of Project Settings.


#include "Slotable.h"

#include "Constituent.h"
#include "Inventory.h"
#include "Net/UnrealNetwork.h"

USlotable::USlotable()
{
}

void USlotable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(USlotable, Constituents, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(USlotable, OwningInventory, DefaultParams);
}

const TArray<UConstituent*>& USlotable::GetConstituents() const
{
	return Constituents;
}

TArray<UConstituent*> USlotable::GetConstituentsOfClass(const TSubclassOf<UConstituent>& InConstituentClass)
{
	if (!InConstituentClass.Get())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called GetConstituentsOfClass with empty TSubclassOf on USlotable class %s."), *GetClass()->GetName());
	}
	TArray<UConstituent*> ConstituentsOfClass;
	for (UConstituent* Constituent : Constituents)
	{
		if (Constituent->GetClass() == InConstituentClass)
		{
			ConstituentsOfClass.Add(Constituent);
		}
	}
	return ConstituentsOfClass;
}

void USlotable::AutonomousInitialize()
{
	Autonomous_Initialize();
}

void USlotable::ServerInitialize()
{
	if (InitialConstituentClasses.Num() > 32)
	{
		UE_LOG(LogSfCore, Error, TEXT("More than 32 constituents are in USlotable class %s. Removing excess."), *GetClass()->GetName());
		while (InitialConstituentClasses.Num() > 32)
		{
			InitialConstituentClasses.RemoveAt(33, 1, false);
		}
		InitialConstituentClasses.Shrink();
	}
	//We don't instantly init as on the server we need to make sure the slotable is in an inventory first.
	Constituents.Reserve(InitialConstituentClasses.Num());
	for (auto ConstituentClass : InitialConstituentClasses)
	{
		UConstituent* ConstituentInstance = CreateUninitializedConstituent(ConstituentClass);
		Constituents.Add(ConstituentInstance);
		MARK_PROPERTY_DIRTY_FROM_NAME(USlotable, Constituents, this);
	}
	for (UConstituent* Constituent : Constituents)
	{
		ServerInitializeConstituent(Constituent);
	}
	Server_Initialize();
	ClientAutonomousInitialize(OwningInventory);
}

void USlotable::AutonomousDeinitialize()
{
	Autonomous_Deinitialize();
}

void USlotable::ServerDeinitialize()
{
	Server_Deinitialize();
	ClientAutonomousDeinitialize();
	for (UConstituent* Constituent : Constituents)
	{
		ServerDeinitializeConstituent(Constituent);
	}
}

void USlotable::ClientAutonomousInitialize_Implementation(UInventory* InOwningInventory)
{
	OwningInventory = InOwningInventory;
	AutonomousInitialize();
}

void USlotable::ClientAutonomousDeinitialize_Implementation()
{
	AutonomousDeinitialize();
}

void USlotable::AssignConstituentInstanceId(UConstituent* Constituent)
{
	OwningInventory->AssignConstituentInstanceId(Constituent);
}

void USlotable::ServerInitializeConstituent(UConstituent* Constituent)
{
	GetOwner()->AddReplicatedSubObject(Constituent);
	Constituent->OwningSlotable = this;
	AssignConstituentInstanceId(Constituent);
	MARK_PROPERTY_DIRTY_FROM_NAME(UConstituent, OwningSlotable, Constituent);
	Constituent->ServerInitialize();
}

void USlotable::ServerDeinitializeConstituent(UConstituent* Constituent)
{
	Constituent->ServerDeinitialize();
	//Constituent instance ids are recycled automatically.
	GetOwner()->RemoveReplicatedSubObject(Constituent);
}

void USlotable::OnRep_Constituents()
{
	//Register and deregister subobjects on client.
	for (UConstituent* ReplicatedConstituent : Constituents)
	{
		if (!ReplicatedConstituent) continue;
		if (!ClientSubObjectListRegisteredConstituents.Contains(ReplicatedConstituent) && GetOwner())
		{
			GetOwner()->AddReplicatedSubObject(ReplicatedConstituent);
			ClientSubObjectListRegisteredConstituents.Add(ReplicatedConstituent);
		}
	}
	for (int16 i = ClientSubObjectListRegisteredConstituents.Num() - 1; i >= 0; i--)
	{
		if (!Constituents.Contains(ClientSubObjectListRegisteredConstituents[i]) && GetOwner())
		{
			GetOwner()->RemoveReplicatedSubObject(ClientSubObjectListRegisteredConstituents[i]);
			ClientSubObjectListRegisteredConstituents.RemoveAt(i, 1, false);
		}
	}
	ClientSubObjectListRegisteredConstituents.Shrink();
}

UConstituent* USlotable::CreateUninitializedConstituent(const TSubclassOf<UConstituent>& InConstituentClass) const
{
	if (!HasAuthority())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called CreateUninitializedConstituent on USlotable class %s without authority."),
			   *GetClass()->GetName());
		return nullptr;
	}
	if (!InConstituentClass.Get())
	{
		UE_LOG(LogSfCore, Error, TEXT("Called CreateUninitializedConstituent with empty TSubclassOf on USlotable class %s."), *GetClass()->GetName());
		return nullptr;
	}
	if (InConstituentClass->HasAnyClassFlags(CLASS_Abstract))
	{
		UE_LOG(LogSfCore, Error, TEXT("Called CreateUninitializedConstituent with TSubclassOf with Abstract flag on USlotable class %s."), *GetClass()->GetName());
		return nullptr;
	}
	UConstituent* ConstituentInstance = NewObject<UConstituent>(GetOwner(), InConstituentClass);
	if (!ConstituentInstance)
	{
		UE_LOG(LogSfCore, Error, TEXT("Failed to create UConstituent instance on USlotable class %s."), *GetClass()->GetName());
	}
	return ConstituentInstance;
}