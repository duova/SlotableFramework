// Fill out your copyright notice in the Description page of Project Settings.


#include "SfTestRunner.h"

#include "GauntletModule.h"
#include "Net/UnrealNetwork.h"

ASfTestRunner::ASfTestRunner()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bReplicateUsingRegisteredSubObjectList = true;
}

void ASfTestRunner::BeginPlay()
{
	Super::BeginPlay();
	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		UE_LOG(LogGauntlet, Display, TEXT("Test runner spawned as AutonomousProxy."));
	}
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		UE_LOG(LogGauntlet, Display, TEXT("Test runner spawned as SimulatedProxy."));
	}
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

void ASfTestRunner::StartTest(TSubclassOf<USfTest> TestClass)
{
}

void ASfTestRunner::EndTest()
{
}

