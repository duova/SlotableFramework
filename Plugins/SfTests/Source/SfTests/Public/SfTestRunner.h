// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SfTestRunner.generated.h"

class USfTest;
/**
 * Replicated actor that is responsible for iterating through all available tests and executing them.
 */
UCLASS()
class SFTESTS_API ASfTestRunner : public AActor
{
	GENERATED_BODY()

public:
	ASfTestRunner();

protected:
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//Register test subobjects on the replication list 1 by 1.
	//In fact have 1 pointer to the currently running test.
	//Server grabs references to all classes at the start, but only initializes them when needed, and they should be instanced.
	//Each test must register its functions to an ordered array of NetRole - function pointer pairs on initialize, this is abstracted
	//by a function that registers them automatically and skips if the net role isn't right.
	//These are called Procedures.
	//The functions will then be executed in that order for those roles.
	//These functions can contain asserts.
	//Function signature returns a float which allows tick to run for a certain period of time before the test continues.
	//Tests need to only run on certain configurations, so if there are 10 clients it would run the 10 client tests instead of the 2 client tests.
	
public:
	virtual void Tick(float DeltaTime) override;

	void StartTest(TSubclassOf<USfTest> TestClass);

	void EndTest();


private:
	UPROPERTY(Replicated)
	USfTest* CurrentTest;
};