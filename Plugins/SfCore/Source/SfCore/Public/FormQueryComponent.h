// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FormQueryComponent.generated.h"

class UFormCoreComponent;
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
	USfQuery();
	
	virtual void PerformCheck(float DeltaTime, UFormCoreComponent* FormCore);

	virtual void Initialize();

	virtual void Deinitialize();
};

//This is an example of how an SfQuery is implemented.
/*
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeath, bool, bDidInFactDie);

UCLASS()
class SFCORE_API UDeathQuery : public USfQuery
{
	GENERATED_BODY()

	UPROPERTY(BlueprintAssignable)
	FOnDeath OnDeath;

public:

	UDeathQuery();
	
	virtual void PerformCheck(float DeltaTime) override;

	virtual void Initialize() override;

	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure)
	static UDeathQuery* CastToDeathQuery(USfQuery* Query);
};
*/

/**
 * The form query component holds instances of subclasses of FSfQuery, and is responsible for their execution. Each
 * constituent may be defined with query dependencies which are automatically added to the component when
 * the constituent is added. The reverse happens on removal. This prevents useless ticked checks from being ran.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SFCORE_API UFormQueryComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class UConstituent;
	
public:
	UFormQueryComponent();

	void RegisterQueryDependencies(const TArray<TSubclassOf<USfQuery>>& Queries);

	void UnregisterQueryDependencies(const TArray<TSubclassOf<USfQuery>>& Queries);
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	void SetupFormQuery(UFormCoreComponent* InFormCore);

protected:
	virtual void BeginPlay() override;

	//Active queries and the number of constituents that depend on it.
	UPROPERTY()
	TMap<USfQuery*, uint16> ActiveQueryDependentCountPair;
	
	UPROPERTY()
	UFormCoreComponent* FormCore;

private:

	void RegisterQueryImpl(TSubclassOf<USfQuery> QueryClass);

	void UnregisterQueryImpl(TSubclassOf<USfQuery> QueryClass);
};
