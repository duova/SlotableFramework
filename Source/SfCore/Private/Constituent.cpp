// Fill out your copyright notice in the Description page of Project Settings.


#include "Constituent.h"

#include "Net/UnrealNetwork.h"

UConstituent::UConstituent()
{
	if (!HasAuthority())
	{
		ClientInitialize();
	}
}

void UConstituent::BeginDestroy()
{
	Super::BeginDestroy();
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
