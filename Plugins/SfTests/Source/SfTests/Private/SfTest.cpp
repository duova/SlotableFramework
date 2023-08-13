// Fill out your copyright notice in the Description page of Project Settings.


#include "SfTest.h"

#include "GauntletModule.h"
#include "SfTestRunner.h"
#include "Net/UnrealNetwork.h"

FTestProcedure::FTestProcedure(): NetRole(), PostProcedureWaitForSeconds(0)
{
}

FTestProcedure::FTestProcedure(const FTestProcedureDelegate& InProcedureDelegate, const ENetRole InNetRole, const float InPostProcedureWaitForSeconds):
	PostProcedureWaitForSeconds(0)
{
	ProcedureDelegate = InProcedureDelegate;
	NetRole = InNetRole;
	PostProcedureWaitForSeconds = InPostProcedureWaitForSeconds;
}

USfTest::USfTest()
{
}

void USfTest::ServerExecute()
{
	RegisterProcedures();
	BlueprintRegisterProcedures();
	UE_LOG(LogGauntlet, Display, TEXT("%s registered %d procedure(s) on server."), *GetClass()->GetName(), Procedures.Num());
	if (Procedures.Num() == 0)
	{
		TestRunner->EndTest(TestCanPass);
		return;
	}
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

int32 USfTest::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	if (GetOwner()->HasAuthority()) return Super::GetFunctionCallspace(Function, Stack);
	return GetOuter()->GetWorld()->GetFirstPlayerController()->GetFunctionCallspace(Function, Stack);
}

bool USfTest::CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack)
{
	if (GetOwner()->HasAuthority()) return Super::CallRemoteFunction(Function, Parms, OutParms, Stack);
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		UE_LOG(LogGauntlet, Error, TEXT("Attempted to call remote function on CDO for USfTest class %s."),
			   *GetClass()->GetName());
		return false;
	}
	if (AActor* PlayerController = GetOuter()->GetWorld()->GetFirstPlayerController())
	{
		PlayerController->GetNetDriver()->ProcessRemoteFunction(PlayerController, Function, Parms, OutParms, Stack, this);
		return true;
	}
	return false;
}

void USfTest::RegisterProcedures()
{
	
}

void USfTest::AddProcedure(const FTestProcedureDelegate& InProcedureDelegate, const ENetRole InNetRole, const float InPostProcedureWaitForSeconds)
{
	Procedures.Emplace(InProcedureDelegate, InNetRole, InPostProcedureWaitForSeconds);
}

void USfTest::OnServerInit()
{
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

void USfTest::GenericAssert(const bool bInSuccess, const FString& SuccessText, const FString& FailText)
{
	if (bInSuccess)
	{
		UE_LOG(LogGauntlet, Display, TEXT("%s Assert Suceeded: %s"),*GetClass()->GetName(), *SuccessText);
		return;
	}
	UE_LOG(LogGauntlet, Error, TEXT("%s Assert Failed: %s"), *GetClass()->GetName(), *FailText);
	TestCanPass = false;
}

void USfTest::AssertBool(const bool RuntimeValue, const bool ExpectedValue, const FString& Name)
{
	if (RuntimeValue == ExpectedValue)
	{
		UE_LOG(LogGauntlet, Display, TEXT("%s Assert Suceeded In %s"), *Name, *GetClass()->GetName());
		return;
	}
	UE_LOG(LogGauntlet, Error, TEXT("%s Assert Failed In %s: Expected %s was %s."), *Name, *GetClass()->GetName(),
	       ExpectedValue ? TEXT("True") : TEXT("False"), RuntimeValue ? TEXT("True") : TEXT("False"));
	TestCanPass = false;
}

void USfTest::AssertInt(const int32 RuntimeValue, const int32 ExpectedValueMinInclusive, const int32 ExpectedValueMaxInclusive, const FString& Name)
{
	if (RuntimeValue >= ExpectedValueMinInclusive && RuntimeValue <= ExpectedValueMaxInclusive)
	{
		UE_LOG(LogGauntlet, Display, TEXT("%s Assert Suceeded In %s"), *Name, *GetClass()->GetName());
		return;
	}
	UE_LOG(LogGauntlet, Error, TEXT("%s Assert Failed In %s: Expected between %d and %d was %d."), *Name,
	       *GetClass()->GetName(), ExpectedValueMinInclusive, ExpectedValueMaxInclusive, RuntimeValue);
	TestCanPass = false;
}

void USfTest::AssertFloat(const float RuntimeValue, const float ExpectedValueMinInclusive, const float ExpectedValueMaxInclusive, const FString& Name)
{
	if (RuntimeValue >= ExpectedValueMinInclusive && RuntimeValue <= ExpectedValueMaxInclusive)
	{
		UE_LOG(LogGauntlet, Display, TEXT("%s Assert Suceeded In %s"), *Name, *GetClass()->GetName());
		return;
	}
	UE_LOG(LogGauntlet, Error, TEXT("%s Assert Failed In %s: Expected between %f and %f was %f."), *Name,
		   *GetClass()->GetName(), ExpectedValueMinInclusive, ExpectedValueMaxInclusive, RuntimeValue);
	TestCanPass = false;
}

void USfTest::Log(const FString& Message)
{
	UE_LOG(LogGauntlet, Display, TEXT("%s %s: %s"), *ASfTestRunner::GetNetRoleAsString(GetOwner()->GetLocalRole()), *GetClass()->GetName(), *Message);
}

void USfTest::ClientExecute_Implementation()
{
	if (GetOwner()->HasAuthority()) return;
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
		ServerClientBeginProcedureCallback();
		Procedures[CurrentProcedureIndex].ProcedureDelegate.Execute();
		//Limit min time until procedure end to 0.1 to ensure all client are able to perform the begin procedure callback
		//before any client calls the finish callback. This is to prevent the server from continuing to the next procedure
		//before all clients are finished.
		LocalTimeUntilProcedureEnd = FMath::Max(Procedures[CurrentProcedureIndex].PostProcedureWaitForSeconds, 0.1);
		bLocalPendingEndProcedure = true;
	}
}

void USfTest::ExecuteServerProcedureIfCorrectNetRole()
{
	if (Procedures[CurrentProcedureIndex].NetRole == ROLE_Authority)
	{
		//Execute procedure and await the timer.
		Procedures[CurrentProcedureIndex].ProcedureDelegate.Execute();
		LocalTimeUntilProcedureEnd = Procedures[CurrentProcedureIndex].PostProcedureWaitForSeconds;
		bLocalPendingEndProcedure = true;
	}
	if (Procedures[CurrentProcedureIndex].NetRole == ROLE_None)
	{
		LocalTimeUntilProcedureEnd = Procedures[CurrentProcedureIndex].PostProcedureWaitForSeconds;
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
			TestRunner->EndTest(TestCanPass);
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
		ServerClientFinishProcedureCallback();
	}
}

//Called on server.
void USfTest::ServerClientBeginProcedureCallback_Implementation()
{
	//We increment an int to track the number of clients that are currently running a certain procedure.
	ClientsRunningCurrentProcedure++;
}

//Called on server.
void USfTest::ServerClientFinishProcedureCallback_Implementation()
{
	//We decrease the int when clients are finished with the procedure to know when all clients are finished with the procedure.
	ClientsRunningCurrentProcedure--;
	if (ClientsRunningCurrentProcedure != 0) return;
	//To prevent a situation where a client calls the begin and end callback in a simultaneous way such that the server believes
	//only one client exists and skips to the next procedure, the min time of a client procedure is 0.1 seconds.
	//Start next procedure.
	CurrentProcedureIndex++;
	if (CurrentProcedureIndex >= Procedures.Num())
	{
		TestRunner->EndTest(TestCanPass);
		return;
	}
	MARK_PROPERTY_DIRTY_FROM_NAME(USfTest, CurrentProcedureIndex, this);
}

void USfTest::InternalOnClientInit_Implementation()
{
	if (GetOwner()->HasAuthority()) return;
	OnClientInit();
	RegisterProcedures();
	BlueprintRegisterProcedures();
	UE_LOG(LogGauntlet, Display, TEXT("%s registered %d procedure(s) on client."), *GetClass()->GetName(), Procedures.Num());
}

void USfTest::InternalOnClientDeinit_Implementation()
{
	if (GetOwner()->HasAuthority()) return;
	OnClientDeinit();
}
