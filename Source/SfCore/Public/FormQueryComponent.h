// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FormQueryComponent.generated.h"

/**
 * SfQueries are to be extended in native code and they exist to perform ticked checks for constituents, providing a modular
 * way for constituents to have ticked event calls. Each SfQuery must declare its own dynamic delegate and create a member that
 * constituents can bind to. PerformCheck needs to be implemented to execute that delegate when the condition the query checks
 * for is true. Only execute the delegate when something has changed. The actual creation of the SfQuery when necessary and
 * ticking of the PerformCheck function is handled by the FormQueryComponent.
 */
UCLASS()
class SFCORE_API USfQuery : public UObject
{
	GENERATED_BODY()

public:
	virtual void PerformCheck(float DeltaTime);

	virtual void Initialize();

	virtual void Deinitialize();
};

/**
 * The form query component holds instances of subclasses of FSfQuery, and is responsible for their execution. Each
 * constituent may be defined with query dependencies which are automatically added to the component when
 * the constituent is added. The reverse happens on removal. This prevents useless ticked checks from being ran.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SFCORE_API UFormQueryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UFormQueryComponent();

	void RegisterQueryDependencies(const TArray<TSubclassOf<USfQuery>>& Queries);

	void UnregisterQueryDependencies(const TArray<TSubclassOf<USfQuery>>& Queries);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	//Active queries and the number of constituents that depend on it.
	UPROPERTY()
	TMap<USfQuery*, uint16> ActiveQueryDependentCountPair;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void RegisterQueryImpl(TSubclassOf<USfQuery> QueryClass);

	void UnregisterQueryImpl(TSubclassOf<USfQuery> QueryClass);
};
