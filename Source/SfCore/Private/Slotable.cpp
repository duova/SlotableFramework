// Fill out your copyright notice in the Description page of Project Settings.


#include "Slotable.h"

#include "Constituent.h"
#include "Net/UnrealNetwork.h"

USlotable::USlotable()
{
	if (HasAuthority())
	{
		//We don't instantly init as on the server we need to make sure the slotable is in an inventory first.
		for (auto ConstituentClass : InitialConstituentClasses)
		{
			UConstituent* ConstituentInstance = CreateUninitializedConstituent(ConstituentClass);
			Constituents.Emplace(ConstituentInstance);
			MARK_PROPERTY_DIRTY_FROM_NAME(USlotable, Constituents, this);
		}
	}
	else
	{
		bAwaitingClientInit = true;
	}
}

void USlotable::Tick(float DeltaTime)
{
	for (UConstituent* Constituent : Constituents)
	{
		Constituent->Tick(DeltaTime);
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

TArray<UConstituent*> USlotable::GetConstituents()
{
	TArray<UConstituent*> ConstituentsCopy;
	for (UConstituent* Constituent : Constituents)
	{
		ConstituentsCopy.Add(Constituent);
	}
	return ConstituentsCopy;
}

void USlotable::ClientInitialize()
{
	//Call events.
}

void USlotable::ServerInitialize()
{
	for (UConstituent* Constituent : Constituents)
	{
		ServerInitializeConstituent(Constituent);
	}
	//Call events.
}

void USlotable::ClientDeinitialize()
{
	//Call events.
}

void USlotable::ServerDeinitialize()
{
	//Call events.
	for (UConstituent* Constituent : Constituents)
	{
		ServerDeinitializeConstituent(Constituent);
	}
}

void USlotable::BeginDestroy()
{
	Super::BeginDestroy();
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
	MARK_PROPERTY_DIRTY_FROM_NAME(UConstituent, OwningSlotable, Constituent);
	Constituent->ServerInitialize();
}

void USlotable::ServerDeinitializeConstituent(UConstituent* Constituent)
{
	Constituent->ServerDeinitialize();
	Constituent->OwningSlotable = nullptr;
	MARK_PROPERTY_DIRTY_FROM_NAME(UConstituent, OwningSlotable, Constituent);
	GetOwner()->RemoveReplicatedSubObject(Constituent);
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
