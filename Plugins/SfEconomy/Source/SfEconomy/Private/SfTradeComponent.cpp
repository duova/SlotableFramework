// Fill out your copyright notice in the Description page of Project Settings.


#include "SfTradeComponent.h"

#include "Net/UnrealNetwork.h"


// Sets default values for this component's properties
USfTradeComponent::USfTradeComponent()
{
	if (!GetOwner()) return;
	PrimaryComponentTick.bCanEverTick = false;
	bReplicateUsingRegisteredSubObjectList = true;
	SetIsReplicated(true);
}

void USfTradeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(USfTradeComponent, HeldCurrencyValues, DefaultParams);
}


// Called when the game starts
void USfTradeComponent::BeginPlay()
{
	Super::BeginPlay();
	
}


// Called every frame
void USfTradeComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

