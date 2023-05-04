// Fill out your copyright notice in the Description page of Project Settings.


#include "Constituent.h"

#include "Net/UnrealNetwork.h"

void UConstituent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(UConstituent, OwningSlotable, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UConstituent, ActivationState, DefaultParams);
}

void UConstituent::Initialize()
{
	//Call events.
}

void UConstituent::Deinitialize()
{
	//Call events.
}
