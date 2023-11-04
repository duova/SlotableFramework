// Fill out your copyright notice in the Description page of Project Settings.


#include "SfTargetingLibrary.h"

#include "FormAnimComponent.h"
#include "FormCharacterComponent.h"
#include "FormCoreComponent.h"

bool USfTargetingLibrary::Predicted_SfMultiLineTrace(UConstituent* Target, const FVector Start, const FVector End,
                                                     ETraceTypeQuery TraceChannel, bool bTraceComplex,
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
	UKismetSystemLibrary::SphereTraceMulti(Target->GetWorld(), Start, End, MaxCompensationRadius, TraceChannel, bTraceComplex, ActorsToIgnore, DrawDebugType, TargetsWithinCompensationRange, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);

	//Rollback potential targets to world time indicated by the client.
	for (FHitResult& PotentialTarget : TargetsWithinCompensationRange)
	{
		AActor* Actor = PotentialTarget.GetActor();
		if (UFormCoreComponent* FormCore = Cast<UFormCoreComponent>(Actor->GetComponentByClass(UFormCoreComponent::StaticClass())))
		{
			FormCore->ServerRollbackLocation(ClientSubmittedWorldTime);
			if (UFormAnimComponent* AnimComp = UFormAnimComponent::GetFormAnimComponent(FormCore))
			{
				AnimComp->ServerRollbackPose(ClientSubmittedWorldTime);
			}
		}
	}

	//Run the actual line trace.
	UKismetSystemLibrary::LineTraceMulti(Target->GetWorld(), Start, End, TraceChannel, bTraceComplex, ActorsToIgnore, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);

	//Return potential targets to original states.
	for (FHitResult& PotentialTarget : TargetsWithinCompensationRange)
	{
		AActor* Actor = PotentialTarget.GetActor();
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
