// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfUtility.h"
#include "Engine/TriggerBox.h"
#include "RelevancyArea.generated.h"

/* 
 * Forms are only relevant to other forms if they are in the same ARelevancyArea or areas that are
 * in the VisibleFrom list of each other.
 */
UCLASS()
class SFCORE_API ARelevancyArea : public ATriggerBox
{
	GENERATED_BODY()

public:
	ARelevancyArea();
	
	FOnEnterOverlapSignature OnEnterOverlapDelegate;
	FOnExitOverlapSignature OnExitOverlapDelegate;

	//Relevancy areas that can see this relevancy area.
	UPROPERTY()
	TArray<ARelevancyArea*> VisibleFrom;

	bool Contains(const AActor* Actor) const;

protected:
	virtual void BeginPlay() override;

	virtual void OnEnterOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
							UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
							const FHitResult& SweepResult);

	virtual void OnExitOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
							   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY()
	TSet<AActor*> ActorsInArea;
};
