// Fill out your copyright notice in the Description page of Project Settings.


#include "SfTestRunner.h"

#include "Net/UnrealNetwork.h"

ASfTestRunner::ASfTestRunner()
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);
	bReplicateUsingRegisteredSubObjectList = true;
}

void ASfTestRunner::BeginPlay()
{
	Super::BeginPlay();
	
}

void ASfTestRunner::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(ASfTestRunner, CurrentTest, DefaultParams);
}

void ASfTestRunner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

