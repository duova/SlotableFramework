// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "SfArea.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSfTargeting, Log, All);

class UConstituent;
class ASfArea;

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FAreaOverlap, AActor*, Actor, ASfArea*, Area, FVector, TargeterLocation);

DECLARE_DYNAMIC_DELEGATE_SixParams(FOnEnterOverlapSignature, UPrimitiveComponent*, OverlappedComponent, AActor*,
                                   OtherActor, UPrimitiveComponent*, OtherComp, int32, OtherBodyIndex, bool, bFromSweep,
                                   const FHitResult &, SweepResult);

DECLARE_DYNAMIC_DELEGATE_FourParams(FOnExitOverlapSignature, UPrimitiveComponent*, OverlappedComponent, AActor*,
                                    OtherActor, UPrimitiveComponent*, OtherComp, int32, OtherBodyIndex);

/*
 * Note: This has to be implemented with a UPrimitiveComponent to function.
 * AActor that can be directly spawned with SpawnArea on a UConstituent that is used for overlap detection.
 * This exists so that all logic relating on a UConstituent stays on a UConstituent instead of being on multiple UObjects.
 */
UCLASS(Abstract, Blueprintable)
class SFTARGETING_API ASfArea : public AActor
{
	GENERATED_BODY()

public:
	ASfArea();

	//Spawns an implemented ASfArea.
	//Tick is disabled if TickInterval is 0. It should not be anything less than 0.1 for performance.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta = (DefaultToSelf = "Target", AutoCreateRefTerm = "InActorsToIgnore, InTeamsToIgnore, OnEnterEvent, OnExitEvent, OnTickEvent"))
	static ASfArea* SpawnArea(UConstituent* Target, const TSubclassOf<ASfArea>& InClass,
	                          const FVector& InLocation, const FRotator& InRotation,
	                          const TArray<AActor*>& InActorsToIgnore,
	                          const TArray<FGameplayTag>& InTeamsToIgnore,
	                          const float InTickInterval,
	                          const FAreaOverlap& OnEnterEvent, const FAreaOverlap& OnExitEvent,
	                          const FAreaOverlap& OnTickEvent, const bool bInOnlyTargetForms = true);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<AActor*> GetActorsInArea() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TSet<AActor*> ActorsToIgnore;

	TSet<FGameplayTag> TeamsToIgnore;

	UPROPERTY()
	TSet<AActor*> OverlappingActors;

	UPROPERTY()
	UPrimitiveComponent* PrimitiveComponent;

	bool bOnlyTargetForms;

	//For UPrimitiveComponent.
	FOnEnterOverlapSignature OnEnterOverlapDelegate;
	FOnExitOverlapSignature OnExitOverlapDelegate;

	//For UConstituent.
	FAreaOverlap OnEnter;
	FAreaOverlap OnExit;
	FAreaOverlap OnTick;

	virtual void OnEnterOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                            UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                            const FHitResult& SweepResult);

	virtual void OnExitOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                           UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:
	virtual void Tick(float DeltaTime) override;
};
