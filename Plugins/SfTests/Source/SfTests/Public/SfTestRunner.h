// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SfTestRunner.generated.h"

class USfGauntletController;
class USfTest;

/**
 * Replicated actor that is responsible for iterating through all available tests and executing them.
 */
UCLASS()
class SFTESTS_API ASfTestRunner : public APawn
{
	GENERATED_BODY()

public:
	ASfTestRunner();

protected:
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
public:
	virtual void Tick(float DeltaTime) override;

	bool StartTest(const TSubclassOf<USfTest> TestClass);

	void EndTest(const bool bPassed);

	UPROPERTY()
	USfGauntletController* ServerSfGauntletController;

	static FString GetNetRoleAsString(const ENetRole NetRole);

	bool bIsPassing = true;

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastEndTestSession(const bool bPassed);

	UPROPERTY(Replicated)
	uint8 NumClients;

private:
	UPROPERTY(Replicated)
	USfTest* CurrentTest;

	UPROPERTY(Replicated)
	double TimeSinceStart;

	static constexpr float ConnectionWaitInSeconds = 3.f;

	TArray<UClass*> ServerIndexedTestsToRun;

	bool ServerSetupPerformed = false;

	uint8 NextTestIndex = 0;

	FString DefaultMapName;

	bool bServerKillWhenEmpty = false;

	float ServerAwaitClientDisconnectTimeout = 2.f;
};