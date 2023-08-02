// Fill out your copyright notice in the Description page of Project Settings.


#include "SfTest.h"

#include "SfTestRunner.h"
#include "Net/UnrealNetwork.h"

FTestProcedure::FTestProcedure(): ProcedureFunction(nullptr), NetRole()
{
}

FTestProcedure::FTestProcedure(float (*InProcedureFunction)(), const ENetRole InNetRole): ProcedureFunction(nullptr),
	NetRole()
{
	ProcedureFunction = InProcedureFunction;
	NetRole = InNetRole;
}

USfTest::USfTest()
{
}

void USfTest::ServerExecute()
{
	RegisterProcedures();
	ClientExecute();
	bStartProcedures = true;
	ExecuteServerProcedureIfCorrectNetRole();
}

void USfTest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(USfTest, CurrentProcedureIndex, DefaultParams);
}

void USfTest::OnServerInit()
{
}

void USfTest::RegisterProcedures()
{
}

void USfTest::AddProcedure(float (*InProcedureFunction)(), ENetRole InNetRole)
{
	Procedures.Emplace(InProcedureFunction, InNetRole);
}

void USfTest::OnServerDeinit()
{
}

void USfTest::OnClientInit()
{
}

void USfTest::OnClientDeinit()
{
}

void USfTest::Tick(const float DeltaTime)
{
}

void USfTest::ClientExecute_Implementation()
{
	bStartProcedures = true;
	ExecuteClientProcedureIfCorrectNetRole();
}

void USfTest::OnRep_CurrentProcedureIndex()
{
	//Execute procedure locally if procedure index incremented.
	if (bStartProcedures) ExecuteClientProcedureIfCorrectNetRole();
}

void USfTest::ExecuteClientProcedureIfCorrectNetRole()
{
	//If we aren't the server and the net role is the one specified, run callbacks and procedure.
	if (GetOwner()->GetLocalRole() == ROLE_Authority) return;
	if (GetOwner()->GetLocalRole() == Procedures[CurrentProcedureIndex].NetRole)
	{
		ClientBeginProcedureCallback();
		//Limit min time until procedure end to 0.2 to ensure all client are able to perform the begin procedure callback
		//before any client calls the finish callback. This is to prevent the server from continuing to the next procedure
		//before all clients are finished.
		LocalTimeUntilProcedureEnd = FMath::Max(Procedures[CurrentProcedureIndex].ProcedureFunction(), 0.2);
		bLocalPendingEndProcedure = true;
	}
}

void USfTest::ExecuteServerProcedureIfCorrectNetRole()
{
	if (Procedures[CurrentProcedureIndex].NetRole == ROLE_Authority)
	{
		//Execute procedure and await the timer.
		LocalTimeUntilProcedureEnd = Procedures[CurrentProcedureIndex].ProcedureFunction();
		bLocalPendingEndProcedure = true;
	}
	//Otherwise the server waits for the client callbacks.
}

void USfTest::ServerTick(const float DeltaTime)
{
	//Increment the current procedure and execute locally after the post procedure timer has ended if it exists.
	if (!bLocalPendingEndProcedure) return;
	LocalTimeUntilProcedureEnd -= DeltaTime;
	if (LocalTimeUntilProcedureEnd < 0.f)
	{
		ClientsRunningCurrentProcedure = 0;
		CurrentProcedureIndex++;
		bLocalPendingEndProcedure = false;
		if (CurrentProcedureIndex >= Procedures.Num())
		{
			TestRunner->EndTest();
			return;
		}
		MARK_PROPERTY_DIRTY_FROM_NAME(USfTest, CurrentProcedureIndex, this);
		ExecuteServerProcedureIfCorrectNetRole();
	}
}

void USfTest::ClientTick(const float DeltaTime)
{
	//Execute the finish callback after the post procedure timer has ended if it exists.
	if (!bLocalPendingEndProcedure) return;
	LocalTimeUntilProcedureEnd -= DeltaTime;
	if (LocalTimeUntilProcedureEnd < 0.f)
	{
		bLocalPendingEndProcedure = false;
		ClientFinishProcedureCallback();
	}
}

void USfTest::ClientBeginProcedureCallback_Implementation()
{
	//We increment an int to track the number of clients that are currently running a certain procedure.
	ClientsRunningCurrentProcedure++;
}

void USfTest::ClientFinishProcedureCallback_Implementation()
{
	//We decrease the int when clients are finished with the procedure to know when all clients are finished with the procedure.
	ClientsRunningCurrentProcedure--;
	if (ClientsRunningCurrentProcedure != 0) return;
	//To prevent a situation where a client calls the begin and end callback in a simultaneous way such that the server believes
	//only one client exists and skips to the next procedure, the min time of a client procedure is 0.2 seconds.
	//Start next procedure.
	CurrentProcedureIndex++;
	if (CurrentProcedureIndex >= Procedures.Num())
	{
		TestRunner->EndTest();
		return;
	}
	MARK_PROPERTY_DIRTY_FROM_NAME(USfTest, CurrentProcedureIndex, this);
}

void USfTest::InternalOnClientInit_Implementation()
{
	OnClientInit();
	RegisterProcedures();
}

void USfTest::InternalOnClientDeinit_Implementation()
{
	OnClientDeinit();
}
