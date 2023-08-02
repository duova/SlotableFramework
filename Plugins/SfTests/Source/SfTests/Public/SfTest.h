// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfObject.h"
#include "UObject/Object.h"
#include "SfTest.generated.h"

USTRUCT()
struct FTestProcedure
{
	GENERATED_BODY()

	float (*ProcedureFunction)();

	ENetRole NetRole;

	FTestProcedure();

	FTestProcedure(float (*InProcedureFunction)(), const ENetRole InNetRole);
};

/**
 * Base class for a test that will be found by the test runner and executed assuming conditions are matched.
 */
UCLASS()
class SFTESTS_API USfTest : public USfObject
{
	GENERATED_BODY()

	friend class ASfTestRunner;

public:

	USfTest();
	
	//Number of clients the session should have for this test to be valid. Child class should write this value in the
	//constructor. Runs for any number of clients if set to 0.
	uint8 NumClientsRequired = 0;

	//The test runner will not find this test if it is disabled. Child class should write this value in the
	//constructor.
	bool TestDisabled = false;
	
	void ServerExecute();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void OnServerInit();

	//Use AddProcedure to add procedures in order.
	virtual void RegisterProcedures();

	//Procedures are function pointers that are executed in order, on the specified net role, and return the to wait
	//in seconds before executing the next procedure.
	virtual void AddProcedure(float (*InProcedureFunction)(), ENetRole InNetRole);

	//Clean up and revert world back to original state on deinit.
	virtual void OnServerDeinit();
	
	virtual void OnClientInit();
	
	virtual void OnClientDeinit();

	virtual void Tick(const float DeltaTime);

private:
	TArray<FTestProcedure> Procedures;

	UPROPERTY()
	ASfTestRunner* TestRunner;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentProcedureIndex)
	uint16 CurrentProcedureIndex = 0;

	uint8 ClientsRunningCurrentProcedure = 0;

	UFUNCTION(NetMulticast, Reliable)
	void InternalOnClientInit();

	UFUNCTION(NetMulticast, Reliable)
	void InternalOnClientDeinit();

	UFUNCTION(NetMulticast, Reliable)
	void ClientExecute();

	UFUNCTION(Server, Reliable)
	void ClientBeginProcedureCallback();

	UFUNCTION(Server, Reliable)
	void ClientFinishProcedureCallback();

	UFUNCTION()
	void OnRep_CurrentProcedureIndex();

	void ExecuteClientProcedureIfCorrectNetRole();

	void ExecuteServerProcedureIfCorrectNetRole();

	void ServerTick(const float DeltaTime);

	void ClientTick(const float DeltaTime);

	bool bStartProcedures = false;

	bool bLocalPendingEndProcedure = false;

	float LocalTimeUntilProcedureEnd = 0;
};
