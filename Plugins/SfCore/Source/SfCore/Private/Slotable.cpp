// Fill out your copyright notice in the Description page of Project Settings.


#include "Slotable.h"

#include "Constituent.h"
#include "Inventory.h"
#include "Net/UnrealNetwork.h"

USlotable::USlotable()
{
	if (!GetOwner()) return;
	if (HasAuthority())
	{
		checkf(InitialConstituentClasses.Num() < 33, TEXT("Only 32 constituents can be in a slotable."));
		//We don't instantly init as on the server we need to make sure the slotable is in an inventory first.
		Constituents.Reserve(InitialConstituentClasses.Num());
		for (auto ConstituentClass : InitialConstituentClasses)
		{
			UConstituent* ConstituentInstance = CreateUninitializedConstituent(ConstituentClass);
			Constituents.Add(ConstituentInstance);
			MARK_PROPERTY_DIRTY_FROM_NAME(USlotable, Constituents, this);
		}
	}
	else
	{
		bAwaitingClientInit = true;
	}
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

const TArray<UConstituent*>& USlotable::GetConstituents()
{
	return Constituents;
}

TArray<UConstituent*> USlotable::GetConstituentsOfClass(const TSubclassOf<UConstituent> ConstituentClass)
{
	TArray<UConstituent*> ConstituentsOfClass;
	for (UConstituent* Constituent : Constituents)
	{
		if (Constituent->GetClass() == ConstituentClass)
		{
			ConstituentsOfClass.Add(Constituent);
		}
	}
	return ConstituentsOfClass;
}

void USlotable::ClientInitialize()
{
	Client_Initialize();
}

void USlotable::ServerInitialize()
{
	for (UConstituent* Constituent : Constituents)
	{
		ServerInitializeConstituent(Constituent);
	}
	Server_Initialize();
}

void USlotable::ClientDeinitialize()
{
	Client_Deinitialize();
}

void USlotable::ServerDeinitialize()
{
	Server_Deinitialize();
	for (UConstituent* Constituent : Constituents)
	{
		ServerDeinitializeConstituent(Constituent);
	}
}

void USlotable::AssignConstituentInstanceId(UConstituent* Constituent)
{
	OwningInventory->AssignConstituentInstanceId(Constituent);
}

void USlotable::BeginDestroy()
{
	Super::BeginDestroy();
	if (!GetOwner()) return;
	if (HasAuthority())
	{
		for (int32 i = 0; i < Constituents.Num(); i++)
		{
			//We clear index 0 because the list shifts down. 
			if (UConstituent* Constituent = Constituents[0])
			{
				ServerDeinitializeConstituent(Constituent);
				//We manually mark the object as garbage so its deletion can be replicated sooner to clients.
				Constituent->Destroy();
			}
			Constituents.RemoveAt(0);
			MARK_PROPERTY_DIRTY_FROM_NAME(USlotable, Constituents, this);
			
		}
	}
	else
	{
		ClientDeinitialize();
	}
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
	Constituent->OwningSlotable = nullptr;
	//Constituent instance ids are recycled automatically.
	MARK_PROPERTY_DIRTY_FROM_NAME(UConstituent, OwningSlotable, Constituent);
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
	TArray<UConstituent*> ToRemove;
	for (UConstituent* RegisteredConstituent : ClientSubObjectListRegisteredConstituents)
	{
		if (!Constituents.Contains(RegisteredConstituent) && GetOwner())
		{
			GetOwner()->RemoveReplicatedSubObject(RegisteredConstituent);
			ToRemove.Add(RegisteredConstituent);
		}
	}
	for (UConstituent* ConstituentToRemove : ToRemove)
	{
		ClientSubObjectListRegisteredConstituents.Remove(ConstituentToRemove);
	}
}

UConstituent* USlotable::CreateUninitializedConstituent(const TSubclassOf<UConstituent>& ConstituentClass) const
{
	ErrorIfNoAuthority();
	if (!ConstituentClass || ConstituentClass->HasAnyClassFlags(CLASS_Abstract)) return nullptr;
	UConstituent* ConstituentInstance = NewObject<UConstituent>(GetOwner(), ConstituentClass);
	checkf(ConstituentInstance, TEXT("Failed to create slotable."));
	return ConstituentInstance;
}

void USlotable::OnRep_OwningInventory()
{
	if (bAwaitingClientInit)
	{
		bAwaitingClientInit = false;
		ClientInitialize();
	}
}
