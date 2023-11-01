// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "SfTargeter.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSfTargeter, Log, All);

class UConstituent;
class ASfTargeter;

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FTargeterOverlap, AActor*, Actor, ASfTargeter*, Targeter, FVector, TargeterLocation);

DECLARE_DYNAMIC_DELEGATE_SixParams(FOnEnterOverlapSignature, UPrimitiveComponent*, OverlappedComponent, AActor*, OtherActor, UPrimitiveComponent*, OtherComp, int32, OtherBodyIndex, bool, bFromSweep, const FHitResult &, SweepResult);
DECLARE_DYNAMIC_DELEGATE_FourParams(FOnExitOverlapSignature, UPrimitiveComponent*, OverlappedComponent, AActor*, OtherActor, UPrimitiveComponent*, OtherComp, int32, OtherBodyIndex);

/*
 * Note: This has to be implemented with a UPrimitiveComponent to function.
 * AActor that can be directly spawned with SpawnTargeter on a UConstituent that is used for overlap detection.
 * This exists so that all logic relating on a UConstituent stays on a UConstituent instead of being on multiple UObjects.
 */
UCLASS(Abstract, Blueprintable)
class SFTARGETING_API ASfTargeter : public AActor
{
	GENERATED_BODY()

public:
	ASfTargeter();

	//Spawns an implemented ASfTargeter.
	//Tick is disabled if TickInterval is 0. It should not be anything less than 0.1 for performance.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta = (DefaultToSelf = "Target"))
	static ASfTargeter* SpawnTargeter(UConstituent* Target, const TSubclassOf<ASfTargeter>& InClass,
	                                  const FVector& InLocation, const FRotator& InRotation,
	                                  const TArray<AActor*>& InActorsToIgnore,
	                                  const TArray<FGameplayTag>& InTeamsToIgnore, const float TickInterval,
	                                  const FTargeterOverlap OnEnterEvent, const FTargeterOverlap OnExitEvent, const FTargeterOverlap OnTickEvent);

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TSet<AActor*> ActorsToIgnore;

	TSet<FGameplayTag> TeamsToIgnore;

	UPROPERTY()
	TSet<AActor*> OverlappingActors;

	UPROPERTY()
	UPrimitiveComponent* PrimitiveComponent;

	//For UPrimitiveComponent.
	FOnEnterOverlapSignature OnEnterOverlapDelegate;
	FOnExitOverlapSignature OnExitOverlapDelegate;

	//For UConstituent.
	FTargeterOverlap OnEnter;
	FTargeterOverlap OnExit;
	FTargeterOverlap OnTick;

	virtual void OnEnterOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	virtual void OnExitOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
public:
	virtual void Tick(float DeltaTime) override;
};
