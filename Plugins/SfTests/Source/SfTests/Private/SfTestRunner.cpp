// Fill out your copyright notice in the Description page of Project Settings.


#include "SfTestRunner.h"

#include "GauntletModule.h"
#include "SfGauntletController.h"
#include "SfTest.h"
#include "Net/UnrealNetwork.h"

ASfTestRunner::ASfTestRunner()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bReplicateUsingRegisteredSubObjectList = true;
	bAlwaysRelevant = true;
}

void ASfTestRunner::BeginPlay()
{
	Super::BeginPlay();
	if (GetLocalRole() == ROLE_Authority)
	{
		UE_LOG(LogGauntlet, Display, TEXT("Test runner spawned as Authority."));
	}
	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		UE_LOG(LogGauntlet, Display, TEXT("Test runner spawned as AutonomousProxy."));
	}
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		UE_LOG(LogGauntlet, Display, TEXT("Test runner spawned as SimulatedProxy."));
	}

	UE_LOG(LogGauntlet, Display, TEXT("Awaiting connections."));
}

void ASfTestRunner::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(ASfTestRunner, CurrentTest, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ASfTestRunner, NumClients, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ASfTestRunner, TimeSinceStart, DefaultParams);
}

void ASfTestRunner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CurrentTest->Tick(DeltaTime);

	//Only run rest of tick on server.
	if (!HasAuthority()) return;

	//Tick time.
	TimeSinceStart += DeltaTime;
	MARK_PROPERTY_DIRTY_FROM_NAME(ASfTestRunner, TimeSinceStart, this);
	
	if (CurrentTest)
	{
		CurrentTest->ServerTick(DeltaTime);
		CurrentTest->ClientTick(DeltaTime);
	}

	//Perform setup.
	if (!ServerSetupPerformed)
	{
		//Wait for connections.
		if (TimeSinceStart <= ConnectionWaitInSeconds) return;

		//Index valid tests based on test configuration.
		NumClients = GetWorld()->PlayerNum;
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (!It->IsChildOf(USfTest::StaticClass())) continue;
			if (*It == USfTest::StaticClass()) continue;
			const USfTest* TestCDO = static_cast<USfTest*>((*It)->GetDefaultObject());
			if (TestCDO->NumClientsRequired != NumClients) return;
			if (TestCDO->TestDisabled == true) return;
			ServerIndexedTestsToRun.Add(*It);
		}
		ServerSetupPerformed = true;
		UE_LOG(LogGauntlet, Display, TEXT("SfTestRunner identified %i tests to run."), ServerIndexedTestsToRun.Num());
	}

	//Post setup.
	if (ServerSetupPerformed)
	{
		//If previous test ended.
		if (CurrentTest == nullptr)
		{
			//End session if no more tests are available.
			if (NextTestIndex >= ServerIndexedTestsToRun.Num())
			{
				ServerSfGauntletController->SfEndSession();
				return;
			}

			//If no test running we start a test.
			StartTest(ServerIndexedTestsToRun[NextTestIndex]);
			NextTestIndex++;
		}
	}
}

bool ASfTestRunner::StartTest(const TSubclassOf<USfTest> TestClass)
{
	if (CurrentTest) return false;
	if (!TestClass.Get()) return false;
	CurrentTest = NewObject<USfTest>(this, TestClass);
	if (!CurrentTest) return false;
	AddReplicatedSubObject(CurrentTest);
	CurrentTest->TestRunner = this;
	CurrentTest->OnServerInit();
	CurrentTest->InternalOnClientInit();
	CurrentTest->ServerExecute();
	UE_LOG(LogGauntlet, Display, TEXT("%s test started."), *TestClass->GetName());
	return true;
}

void ASfTestRunner::EndTest()
{
	if (!CurrentTest) return;
	CurrentTest->OnServerDeinit();
	CurrentTest->InternalOnClientDeinit();
	RemoveReplicatedSubObject(CurrentTest);
	UE_LOG(LogGauntlet, Display, TEXT("%s test ended."), *CurrentTest->GetClass()->GetName());
	CurrentTest = nullptr;
}
