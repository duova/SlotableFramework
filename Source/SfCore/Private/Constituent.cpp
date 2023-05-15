// Fill out your copyright notice in the Description page of Project Settings.


#include "Constituent.h"

#include "Net/UnrealNetwork.h"

FConstituentStateData::FConstituentStateData(): CurrentState(0), PreviousState(0), bClientCorrecting(0)
{
}

FConstituentStateData::FConstituentStateData(const uint8 CurrentState, const uint8 PreviousState, const bool bClientCorrecting):
	CurrentState(CurrentState),
	PreviousState(PreviousState),
	bClientCorrecting(bClientCorrecting)
{
}

UConstituent::UConstituent()
{
	if (!HasAuthority())
	{
		bAwaitingClientInit = true;
	}
}

void UConstituent::BeginDestroy()
{
	Super::BeginDestroy();
	bIsBeingDestroyed = true;
	if (!HasAuthority())
	{
		ClientDeinitialize();
	}
}

void UConstituent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(UConstituent, OwningSlotable, DefaultParams);
}

void UConstituent::OnRep_OwningSlotable()
{
	if (bAwaitingClientInit)
	{
		bAwaitingClientInit = false;
		ClientInitialize();
	}
}

void UConstituent::ClientInitialize()
{
	
}

void UConstituent::ServerInitialize()
{
	//Call events.
}

void UConstituent::ClientDeinitialize()
{
	
}

void UConstituent::ServerDeinitialize()
{
	//Call events.
}
