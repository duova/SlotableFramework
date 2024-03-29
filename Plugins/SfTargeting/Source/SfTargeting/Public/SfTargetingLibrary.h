﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Constituent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/Object.h"
#include "SfTargetingLibrary.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSfTargeting, Log, All);

/**
 * Library for UConstituent targeting functions.
 */
UCLASS(Abstract)
class SFTARGETING_API USfTargetingLibrary : public UObject
{
	GENERATED_BODY()

	//Multi line trace that compensates for latency when targeting forms.
	//This does not return values on the client.
	//BlockingTraceChannel should be blocking on objects that should be blocking for the trace.
	//NonBlockingTraceChannel should not be blocking for any objects, as it is used in a sphere trace to pick up
	//targets in range of lag compensation.
	UFUNCTION(BlueprintCallable, meta = (DefaultToSelf = "Target", bIgnoreSelf= "true", AutoCreateRefTerm = "ActorsToIgnore", AdvancedDisplay= "TraceColor, TraceHitColor, DrawTime"))
	static bool Predicted_SfMultiLineTrace(UConstituent* Target, const FVector Start, const FVector End,
	                                       ETraceTypeQuery BlockingTraceChannel, ETraceTypeQuery NonBlockingTraceChannel, bool bTraceComplex,
	                                       const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType,
	                                       TArray<FHitResult>& OutHits, bool bIgnoreSelf, float MaxCompensationRadius,
	                                       FLinearColor TraceColor = FLinearColor::Red,
	                                       FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	//Single line trace that compensates for latency when targeting forms.
	//This does not return values on the client.
	//BlockingTraceChannel should be blocking on objects that should be blocking for the trace.
	//NonBlockingTraceChannel should not be blocking for any objects, as it is used in a sphere trace to pick up
	//targets in range of lag compensation.
	UFUNCTION(BlueprintCallable, meta = (DefaultToSelf = "Target", bIgnoreSelf= "true", AutoCreateRefTerm = "ActorsToIgnore", AdvancedDisplay= "TraceColor, TraceHitColor, DrawTime"))
	static bool Predicted_SfLineTrace(UConstituent* Target, const FVector Start, const FVector End,
										   ETraceTypeQuery BlockingTraceChannel, ETraceTypeQuery NonBlockingTraceChannel, bool bTraceComplex,
										   const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType,
										   FHitResult& OutHit, bool bIgnoreSelf, float MaxCompensationRadius,
										   FLinearColor TraceColor = FLinearColor::Red,
										   FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
	
	static void RollbackPotentialTargets(const float InClientSubmittedWorldTime, TArray<FHitResult>& TargetsWithinCompensationRange);

	static void RestorePotentialTargets(TArray<FHitResult>& TargetsWithinCompensationRange);
	
	static constexpr float MaxCompensationTimeSeconds = 0.25;
};
