// Fill out your copyright notice in the Description page of Project Settings.


#include "SfTargetingLibrary.h"

#include "FormAnimComponent.h"
#include "FormCharacterComponent.h"
#include "FormCoreComponent.h"

bool USfTargetingLibrary::Predicted_SfMultiLineTrace(UConstituent* Target, const FVector Start, const FVector End,
                                                     ETraceTypeQuery BlockingTraceChannel,
                                                     ETraceTypeQuery NonBlockingTraceChannel, bool bTraceComplex,
                                                     const TArray<AActor*>& ActorsToIgnore,
                                                     EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits,
                                                     bool bIgnoreSelf, float MaxCompensationRadius,
                                                     FLinearColor TraceColor, FLinearColor TraceHitColor,
                                                     float DrawTime)
{
	//This is marked Predicted even though it only has server logic to indicate that it should be client input driven and requires a form character.

	if (!Target->GetOwner()->HasAuthority()) return false;

	const float CurrentServerWorldTime = Target->GetWorld()->TimeSeconds;

	float ClientSubmittedWorldTime = Target->GetFormCoreComponent()->FormCharacter->GetServerWorldTimeOnClient();

	//Reject if client time is in the future or if their ping is too high.
	if (ClientSubmittedWorldTime > CurrentServerWorldTime || ClientSubmittedWorldTime + MaxCompensationTimeSeconds < CurrentServerWorldTime)
	{
		ClientSubmittedWorldTime = CurrentServerWorldTime;
	}

	//Run sphere trace to determine what actors can be hit considering compensation.
	TArray<FHitResult> TargetsWithinCompensationRange;
	UKismetSystemLibrary::SphereTraceMulti(Target->GetWorld(), Start, End, MaxCompensationRadius, NonBlockingTraceChannel, bTraceComplex, ActorsToIgnore, DrawDebugType, TargetsWithinCompensationRange, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);

	RollbackPotentialTargets(ClientSubmittedWorldTime, TargetsWithinCompensationRange);

	//Run the actual line trace.
	const bool RetVal = UKismetSystemLibrary::LineTraceMulti(Target->GetWorld(), Start, End, BlockingTraceChannel, bTraceComplex, ActorsToIgnore, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);

	RestorePotentialTargets(TargetsWithinCompensationRange);

	return RetVal;
}

bool USfTargetingLibrary::Predicted_SfLineTrace(UConstituent* Target, const FVector Start, const FVector End,
                                                ETraceTypeQuery BlockingTraceChannel, ETraceTypeQuery NonBlockingTraceChannel, bool bTraceComplex,
                                                const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit,
                                                bool bIgnoreSelf, float MaxCompensationRadius, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	//This is marked Predicted even though it only has server logic to indicate that it should be client input driven and requires a form character.

	if (!Target->GetOwner()->HasAuthority()) return false;

	const float CurrentServerWorldTime = Target->GetWorld()->TimeSeconds;

	float ClientSubmittedWorldTime = Target->GetFormCoreComponent()->FormCharacter->GetServerWorldTimeOnClient();

	//Reject if client time is in the future or if their ping is too high.
	if (ClientSubmittedWorldTime > CurrentServerWorldTime || ClientSubmittedWorldTime + MaxCompensationTimeSeconds < CurrentServerWorldTime)
	{
		ClientSubmittedWorldTime = CurrentServerWorldTime;
	}

	//Run sphere trace to determine what actors can be hit considering compensation.
	TArray<FHitResult> TargetsWithinCompensationRange;
	UKismetSystemLibrary::SphereTraceMulti(Target->GetWorld(), Start, End, MaxCompensationRadius, NonBlockingTraceChannel, bTraceComplex, ActorsToIgnore, DrawDebugType, TargetsWithinCompensationRange, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);

	RollbackPotentialTargets(ClientSubmittedWorldTime, TargetsWithinCompensationRange);

	//Run the actual line trace.
	const bool RetVal = UKismetSystemLibrary::LineTraceSingle(Target->GetWorld(), Start, End, BlockingTraceChannel, bTraceComplex, ActorsToIgnore, DrawDebugType, OutHit, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);

	RestorePotentialTargets(TargetsWithinCompensationRange);

	return RetVal;
}

void USfTargetingLibrary::RollbackPotentialTargets(const float InClientSubmittedWorldTime, TArray<FHitResult>& TargetsWithinCompensationRange)
{
	//Rollback potential targets to world time indicated by the client.
	for (FHitResult& PotentialTarget : TargetsWithinCompensationRange)
	{
		AActor* Actor = PotentialTarget.GetActor();
		if (UFormCoreComponent* FormCore = Cast<UFormCoreComponent>(Actor->GetComponentByClass(UFormCoreComponent::StaticClass())))
		{
			FormCore->ServerRollbackLocation(InClientSubmittedWorldTime);
			if (UFormAnimComponent* AnimComp = UFormAnimComponent::GetFormAnimComponent(FormCore))
			{
				AnimComp->ServerRollbackPose(InClientSubmittedWorldTime);
			}
		}
	}
}

void USfTargetingLibrary::RestorePotentialTargets(TArray<FHitResult>& TargetsWithinCompensationRange)
{
	//Return potential targets to original states.
	for (FHitResult& PotentialTarget : TargetsWithinCompensationRange)
	{
		const AActor* Actor = PotentialTarget.GetActor();
		if (UFormCoreComponent* FormCore = Cast<UFormCoreComponent>(Actor->GetComponentByClass(UFormCoreComponent::StaticClass())))
		{
			FormCore->ServerRestoreLatestLocation();
			if (UFormAnimComponent* AnimComp = UFormAnimComponent::GetFormAnimComponent(FormCore))
			{
				AnimComp->ServerRestoreLatestPose();
			}
		}
	}
}
