// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfObject.h"
#include "UObject/Object.h"
#include "SfTest.generated.h"

DECLARE_DYNAMIC_DELEGATE(FTestProcedureDelegate);

USTRUCT()
struct FTestProcedure
{
	GENERATED_BODY()

	FTestProcedureDelegate ProcedureDelegate;

	ENetRole NetRole;

	float PostProcedureWaitForSeconds;

	FTestProcedure();

	FTestProcedure(const FTestProcedureDelegate& InProcedureDelegate, const ENetRole InNetRole, const float InPostProcedureWaitForSeconds);
};

/**
 * Base class for a test that will be found by the test runner and executed assuming conditions are matched.
 */
UCLASS(Blueprintable)
class SFTESTS_API USfTest : public USfObject
{
	GENERATED_BODY()

	friend class ASfTestRunner;

public:

	USfTest();
	
	//Number of clients the session should have for this test to be valid. Child class should write this value in the
	//constructor. Runs for any number of clients if set to -1.
	UPROPERTY(EditAnywhere, meta = (ClampMin = -1))
	int32 NumClientsRequired = -1;

	//The test runner will not find this test if it is disabled. Child class should write this value in the
	//constructor.
	UPROPERTY(EditAnywhere)
	bool TestDisabled = false;
	
	void ServerExecute();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//Route through player controller to allow for server rpc calls.
	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;

	//Route through player controller to allow for server rpc calls.
	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack) override;

	//Use AddProcedure to add procedures in order.
	virtual void RegisterProcedures();

	//Use AddProcedure to add procedures in order.
	UFUNCTION(BlueprintImplementableEvent)
	void BlueprintRegisterProcedures();

	//Procedures are delegates that are executed in order, on the specified net role.
	UFUNCTION(BlueprintCallable)
	virtual void AddProcedure(const FTestProcedureDelegate& InProcedureDelegate, const ENetRole InNetRole, const float InPostProcedureWaitForSeconds);
	
	virtual void OnServerInit();

	//Clean up and revert world back to original state on deinit.
	virtual void OnServerDeinit();
	
	virtual void OnClientInit();
	
	virtual void OnClientDeinit();

	virtual void Tick(const float DeltaTime);

	UFUNCTION(BlueprintCallable)
	void GenericAssert(const bool bInSuccess, const FString& SuccessText, const FString& FailText);

	UFUNCTION(BlueprintCallable)
	void AssertBool(const bool RuntimeValue, const bool ExpectedValue, const FString& Name);

	UFUNCTION(BlueprintCallable)
	void AssertInt(const int32 RuntimeValue, const int32 ExpectedValueMinInclusive, const int32 ExpectedValueMaxInclusive, const FString& Name);

	UFUNCTION(BlueprintCallable)
	void AssertFloat(const float RuntimeValue, const float ExpectedValueMinInclusive, const float ExpectedValueMaxInclusive, const FString& Name);

	UFUNCTION(BlueprintCallable)
	void Log(const FString& Message);

	bool TestCanPass = true;

private:
	TArray<FTestProcedure> Procedures;

	UPROPERTY()
	ASfTestRunner* TestRunner;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentProcedureIndex)
	uint16 CurrentProcedureIndex = 0;

	uint8 ClientsRunningCurrentProcedure = 0;

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastInternalOnClientInit();

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastInternalOnClientDeinit();

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastClientExecute();

	UFUNCTION(Server, Reliable)
	void ServerClientBeginProcedureCallback();

	UFUNCTION(Server, Reliable)
	void ServerClientFinishProcedureCallback(const bool bPassed);

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
