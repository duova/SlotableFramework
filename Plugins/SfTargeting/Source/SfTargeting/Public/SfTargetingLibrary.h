// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Constituent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/Object.h"
#include "SfTargetingLibrary.generated.h"

/**
 * Library for UConstituent targeting functions.
 */
UCLASS(Abstract)
class SFTARGETING_API USfTargetingLibrary : public UObject
{
	GENERATED_BODY()

	//Multi line trace that compensates for latency when targeting forms.
	//This does not return values on the client.
	UFUNCTION(BlueprintCallable, meta = (DefaultToSelf = "Target", bIgnoreSelf= "true", AutoCreateRefTerm = "ActorsToIgnore", AdvancedDisplay= "TraceColor, TraceHitColor, DrawTime"))
	static bool Predicted_SfMultiLineTrace(UConstituent* Target, const FVector Start, const FVector End,
	                                       ETraceTypeQuery TraceChannel, bool bTraceComplex,
	                                       const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType,
	                                       TArray<FHitResult>& OutHits, bool bIgnoreSelf, float MaxCompensationRadius,
	                                       FLinearColor TraceColor = FLinearColor::Red,
	                                       FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	//TODO: Make this a CVar
	static constexpr float MaxCompensationTimeSeconds = 0.25;
};
