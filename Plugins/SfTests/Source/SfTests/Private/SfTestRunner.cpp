// Fill out your copyright notice in the Description page of Project Settings.


#include "SfTestRunner.h"

#include "GauntletModule.h"
#include "SfGauntletController.h"
#include "SfTest.h"
#include "GameFramework/GameModeBase.h"
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
	if (IS_PUSH_MODEL_ENABLED())
	{
		UE_LOG(LogGauntlet, Display, TEXT("Push model is enabled."));
	}
	else
	{
		UE_LOG(LogGauntlet, Display, TEXT("Push model is disabled."));
	}
	DefaultMapName = GetWorld()->GetMapName();
	if (GetLocalRole() == ROLE_Authority)
	{
		UE_LOG(LogGauntlet, Display, TEXT("Test runner spawned as Authority."));
		UE_LOG(LogGauntlet, Display, TEXT("Awaiting connections."));
	}
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
	DOREPLIFETIME_WITH_PARAMS_FAST(ASfTestRunner, NumClients, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ASfTestRunner, TimeSinceStart, DefaultParams);
}

void ASfTestRunner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bServerKillWhenEmpty)
	{
		if (GetWorld()->GetAuthGameMode()->GetNumPlayers() == 0)
		{
			USfGauntletController::SfEndSession(bIsPassing);
		}
	}

	if (!HasAuthority() && CurrentTest)
	{
		CurrentTest->ClientTick(DeltaTime);
		CurrentTest->Tick(DeltaTime);
	}
	
	//Only run rest of tick on server.
	if (!HasAuthority()) return;

	//Tick time.
	TimeSinceStart += DeltaTime;
	MARK_PROPERTY_DIRTY_FROM_NAME(ASfTestRunner, TimeSinceStart, this);
	
	if (CurrentTest)
	{
		CurrentTest->ServerTick(DeltaTime);
	}

	//Perform setup.
	if (!ServerSetupPerformed)
	{
		//Wait for connections.
		if (TimeSinceStart <= ConnectionWaitInSeconds) return;

		//Index valid tests based on test configuration.
		NumClients = GetWorld()->GetAuthGameMode()->GetNumPlayers();
		MARK_PROPERTY_DIRTY_FROM_NAME(ASfTestRunner, CurrentTest, this);
		UE_LOG(LogGauntlet, Display, TEXT("Number of players in the world is: %d."), NumClients);

		//Always set one client to autonomous so the test runner can own the tested actor and make it autonomous.
		if (NumClients != 0)
		{
			PlayerController = GetWorld()->GetFirstPlayerController();
			PlayerController->Possess(this);
		}
		
		TArray<UClass*> TestClasses = GetSubclassesOf(USfTest::StaticClass());
		
		for (UClass* Class : TestClasses)
		{
			UE_LOG(LogGauntlet, Display, TEXT("Found class %s."), *Class->GetName());
			if (!Class->IsChildOf(USfTest::StaticClass())) continue;
			if (Class == USfTest::StaticClass()) continue;
			const USfTest* TestCDO = static_cast<USfTest*>(Class->GetDefaultObject());
			if (TestCDO->NumClientsRequired != NumClients && TestCDO->NumClientsRequired != -1) continue;
			if (TestCDO->TestDisabled == true) continue;
			ServerIndexedTestsToRun.Add(Class);
		}
		ServerSetupPerformed = true;
		UE_LOG(LogGauntlet, Display, TEXT("SfTestRunner identified %d tests to run."), ServerIndexedTestsToRun.Num());
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
				NetMulticastEndTestSession(bIsPassing);
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
	MARK_PROPERTY_DIRTY_FROM_NAME(ASfTestRunner, CurrentTest, this);
	if (!CurrentTest) return false;
	AddReplicatedSubObject(CurrentTest);
	CurrentTest->TestRunner = this;
	MARK_PROPERTY_DIRTY_FROM_NAME(USfTest, TestRunner, CurrentTest);
	CurrentTest->OnServerInit();
	CurrentTest->NetMulticastInternalOnClientInit();
	CurrentTest->ServerExecute();
	MARK_PROPERTY_DIRTY_FROM_NAME(ASfTestRunner, CurrentTest, this);
	UE_LOG(LogGauntlet, Display, TEXT("%s test started."), *TestClass->GetName());
	return true;
}

void ASfTestRunner::EndTest(const bool bPassed)
{
	if (!CurrentTest) return;
	CurrentTest->OnServerDeinit();
	CurrentTest->NetMulticastInternalOnClientDeinit();
	RemoveReplicatedSubObject(CurrentTest);
	UE_LOG(LogGauntlet, Display, TEXT("%s test ended."), *CurrentTest->GetClass()->GetName());
	CurrentTest = nullptr;
	MARK_PROPERTY_DIRTY_FROM_NAME(ASfTestRunner, CurrentTest, this);
	bIsPassing &= bPassed;
}

FString ASfTestRunner::GetNetRoleAsString(const ENetRole NetRole)
{
	if (NetRole == ROLE_Authority)
	{
		return FString("Authority");
	}
	if (NetRole == ROLE_AutonomousProxy)
	{
		return FString("AutonomousProxy");
	}
	if (NetRole == ROLE_SimulatedProxy)
	{
		return FString("SimulatedProxy");
	}
	return FString();
}

void ASfTestRunner::NetMulticastEndTestSession_Implementation(const bool bPassed)
{
	if (HasAuthority())
	{
		bServerKillWhenEmpty = true;
	}
	else
	{
		USfGauntletController::SfEndSession(bPassed);
	}
}