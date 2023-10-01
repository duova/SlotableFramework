﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "FormAnimComponent.h"

#include "FormCharacterComponent.h"
#include "FormCoreComponent.h"

DEFINE_LOG_CATEGORY(LogSfAudiovisual);

FRecentMontageData::FRecentMontageData(): Montage(nullptr), StartTimestamp(0), ExpectedEndTimestamp(0),
                                          InterruptedTimestamp(-1.f)
{
}

FRecentMontageData::FRecentMontageData(UAnimMontage* InMontage, const float InStartTimestamp,
                                       const UFormCharacterComponent* InFormCharacterComponent)
{
	Montage = InMontage;
	StartTimestamp = InStartTimestamp;
	ExpectedEndTimestamp = InMontage->bLoop ? -1 : InFormCharacterComponent->CalculateFuturePredictedTimestamp(InMontage->GetPlayLength());
	InterruptedTimestamp = -1.f;
}

UFormAnimComponent::UFormAnimComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	ThirdPersonSkeletalMesh = nullptr;
	FirstPersonSkeletalMesh = nullptr;
	FormCharacterComponent = nullptr;
	bReplaying = false;
}

UFormAnimComponent* UFormAnimComponent::GetFormAnimComponent(UFormCoreComponent* InFormCore)
{
	if (UActorComponent* FormAnim = InFormCore->GetOwner()->FindComponentByClass(StaticClass()))
	{
		return static_cast<UFormAnimComponent*>(FormAnim);
	}
	return nullptr;
}

void UFormAnimComponent::Predicted_PlayMontage(UAnimMontage* InMontage,
                                               const EPerspective InPerspective,
                                               const bool bInStopAllMontages)
{
	InternalPredictedPlayMontage(InMontage, bInStopAllMontages,
	                             InPerspective == EPerspective::FirstPerson);
}

void UFormAnimComponent::Server_PlayMontage(UAnimMontage* InMontage, const bool bInStopAllMontages)
{
	InternalServerPlayMontage(InMontage, bInStopAllMontages);
}

void UFormAnimComponent::Autonomous_PlayMontage(const float InTimeSinceExecution, UAnimMontage* InMontage,
                                                const EPerspective InPerspective,
                                                const bool bInStopAllMontages)
{
	InternalClientPlayMontage(InTimeSinceExecution, InMontage, bInStopAllMontages,
	                          InPerspective == EPerspective::FirstPerson);
}

void UFormAnimComponent::Simulated_PlayMontage(const float InTimeSinceExecution, UAnimMontage* InMontage,
                                               const EPerspective InPerspective,
                                               const bool bInStopAllMontages)
{
	InternalClientPlayMontage(InTimeSinceExecution, InMontage, bInStopAllMontages,
	                          InPerspective == EPerspective::FirstPerson);
}

void UFormAnimComponent::Predicted_StopMontage(UAnimMontage* InMontage,
                                               const EPerspective InPerspective, const float InBlendOutTime)
{
	InternalPredictedStopMontage(InMontage, InPerspective == EPerspective::FirstPerson, InBlendOutTime);
}

void UFormAnimComponent::Server_StopMontage(UAnimMontage* InMontage, const float InBlendOutTime)
{
	InternalServerStopMontage(InMontage, InBlendOutTime);
}

void UFormAnimComponent::Autonomous_StopMontage(const float InTimeSinceExecution, UAnimMontage* InMontage,
                                                const EPerspective InPerspective, const float InBlendOutTime)
{
	InternalClientStopMontage(InTimeSinceExecution, InMontage, InPerspective == EPerspective::FirstPerson,
	                          InBlendOutTime);
}

void UFormAnimComponent::Simulated_StopMontage(const float InTimeSinceExecution, UAnimMontage* InMontage,
                                               const EPerspective InPerspective, const float InBlendOutTime)
{
	InternalClientStopMontage(InTimeSinceExecution, InMontage, InPerspective == EPerspective::FirstPerson,
	                          InBlendOutTime);
}

void UFormAnimComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetOwner()) return;
	FormCharacterComponent = GetOwner()->FindComponentByClass<UFormCharacterComponent>();
	if (FormCharacterComponent)
	{
		ReplayBeginDelegateHandle = FormCharacterComponent->OnBeginRollback.AddUObject(
			this, &UFormAnimComponent::ReplayBegin);
		ReplayEndDelegateHandle = FormCharacterComponent->OnEndRollback.
		                                                  AddUObject(this, &UFormAnimComponent::ReplayEnd);
	}
}

void UFormAnimComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (!GetOwner()) return;
	if (FormCharacterComponent)
	{
		FormCharacterComponent->OnBeginRollback.Remove(ReplayBeginDelegateHandle);
		FormCharacterComponent->OnEndRollback.Remove(ReplayEndDelegateHandle);
	}
}

void UFormAnimComponent::InternalPredictedPlayMontage(UAnimMontage* InMontage,
                                                      const bool bInStopAllMontages,
                                                      const bool bIsFirstPerson)
{
	if (!FormCharacterComponent)
	{
		UE_LOG(LogSfAudiovisual, Error,
		       TEXT("Tried to call Predicted_PlayMontage on UFormAnimComponent without a UFormCharacterComponent."));
		return;
	}

	if (!bReplaying)
	{
		CleanUpIrreleventMontageData(FirstPersonRecentMontages);
		CleanUpIrreleventMontageData(ThirdPersonRecentMontages);
	}

	TArray<FRecentMontageData>& Montages = bIsFirstPerson ? FirstPersonRecentMontages : ThirdPersonRecentMontages;
	UAnimInstance* Instance = bIsFirstPerson ? GetAnimInstanceChecked(FirstPersonSkeletalMesh) : GetAnimInstanceChecked(ThirdPersonSkeletalMesh);

	if (!Instance) return;
	
	for (FRecentMontageData& MontageData : Montages)
	{
		//Check if montage is already playing.
		if (MontageData.Montage == InMontage)
		{
			UE_LOG(LogSfAudiovisual, Warning,
			   TEXT("Tried to call Predicted_PlayMontage on UFormAnimComponent with the same montage already playing. Please stop the montage before playing again."));
			return;
		}
		//If necessary add interrupt time to all other montages.
		if (bInStopAllMontages)
		{
			MontageData.InterruptedTimestamp = FormCharacterComponent->GetPredictedNetClock();
		}
	}

	//Add new montage data. We do this even if replaying as we still have to simulate playing montages.
	Montages.Emplace(InMontage, FormCharacterComponent->GetPredictedNetClock(), FormCharacterComponent);

	//Play montage if not replaying.
	if (!bReplaying)
	{
		Instance->Montage_Play(InMontage, 1.f, EMontagePlayReturnType::MontageLength, 0.f, bInStopAllMontages);
	}
}

void UFormAnimComponent::InternalServerPlayMontage(UAnimMontage* InMontage, const bool bInStopAllMontages)
{
	if (UAnimInstance* Instance = GetAnimInstanceChecked(ThirdPersonSkeletalMesh))
	{
		Instance->Montage_Play(InMontage, 1, EMontagePlayReturnType::MontageLength, 0,
		                       bInStopAllMontages);
	}
}

void UFormAnimComponent::InternalClientPlayMontage(const float InTimeSinceExecution, UAnimMontage* InMontage,
                                                   const bool bInStopAllMontages,
                                                   const bool bIsFirstPerson)
{
	if (UAnimInstance* Instance = bIsFirstPerson
		                              ? GetAnimInstanceChecked(FirstPersonSkeletalMesh)
		                              : GetAnimInstanceChecked(ThirdPersonSkeletalMesh))
	{
		Instance->Montage_Play(InMontage, 1, EMontagePlayReturnType::MontageLength, InTimeSinceExecution,
		                       bInStopAllMontages);
	}
}

void UFormAnimComponent::InternalPredictedStopMontage(UAnimMontage* InMontage,
                                                      const bool bIsFirstPerson, const float InBlendOutTime)
{
	if (!FormCharacterComponent)
	{
		UE_LOG(LogSfAudiovisual, Error,
		       TEXT("Tried to call Stop_PlayMontage on UFormAnimComponent without a UFormCharacterComponent."));
		return;
	}
	
	if (!bReplaying)
	{
		CleanUpIrreleventMontageData(FirstPersonRecentMontages);
		CleanUpIrreleventMontageData(ThirdPersonRecentMontages);
	}
	
	TArray<FRecentMontageData>& Montages = bIsFirstPerson ? FirstPersonRecentMontages : ThirdPersonRecentMontages;
	UAnimInstance* Instance = bIsFirstPerson ? GetAnimInstanceChecked(FirstPersonSkeletalMesh) : GetAnimInstanceChecked(ThirdPersonSkeletalMesh);

	if (!Instance) return;
	
	//Edit montage data.
	for (FRecentMontageData& MontageData : Montages)
	{
		if (MontageData.ExpectedEndTimestamp >= 0 && FormCharacterComponent->CalculateTimeSincePredictedTimestamp(MontageData.ExpectedEndTimestamp) > 0) continue;
		if (MontageData.Montage == InMontage && MontageData.InterruptedTimestamp < 0.f)
		{
			MontageData.InterruptedTimestamp = FormCharacterComponent->GetPredictedNetClock();
		}
	}

	//Stop montage if not replaying.
	if (!bReplaying)
	{
		Instance->Montage_Stop(InBlendOutTime, InMontage);
	}
}

void UFormAnimComponent::InternalServerStopMontage(UAnimMontage* InMontage,
                                                   const float InBlendOutTime)
{
	if (UAnimInstance* Instance = GetAnimInstanceChecked(ThirdPersonSkeletalMesh))
	{
		Instance->Montage_Stop(InBlendOutTime, InMontage);
	}
}

void UFormAnimComponent::InternalClientStopMontage(const float InTimeSinceExecution, UAnimMontage* InMontage,
                                                   const bool bIsFirstPerson, const float InBlendOutTime)
{
	if (UAnimInstance* Instance = bIsFirstPerson
		                              ? GetAnimInstanceChecked(FirstPersonSkeletalMesh)
		                              : GetAnimInstanceChecked(ThirdPersonSkeletalMesh))
	{
		Instance->Montage_Stop(InBlendOutTime, InMontage);
	}
}

void UFormAnimComponent::ReplayBegin()
{
	bReplaying = true;
	
	//Roll back recent montage data to the start of the rollback timeframe.
	RollBackMontageData(FirstPersonRecentMontages, FormCharacterComponent);
	RollBackMontageData(ThirdPersonRecentMontages, FormCharacterComponent);
}

void UFormAnimComponent::ReplayEnd()
{
	UAnimInstance* FirstPersonInstance = GetAnimInstanceChecked(FirstPersonSkeletalMesh);
	UAnimInstance* ThirdPersonInstance = GetAnimInstanceChecked(ThirdPersonSkeletalMesh);

	if (FirstPersonInstance)
	{
		RecreateMontagesFromRecentData(FirstPersonInstance, FirstPersonRecentMontages, FormCharacterComponent);
	}
	if (ThirdPersonInstance)
	{
		RecreateMontagesFromRecentData(ThirdPersonInstance, ThirdPersonRecentMontages, FormCharacterComponent);
	}

	bReplaying = false;
}

void UFormAnimComponent::CleanUpIrreleventMontageData(TArray<FRecentMontageData>& RecentMontages)
{
	for (int16 i = RecentMontages.Num(); i >= 0; i--)
	{
		const float EndTimestamp = RecentMontages[i].InterruptedTimestamp >= 0
									   ? RecentMontages[i].InterruptedTimestamp
									   : RecentMontages[i].ExpectedEndTimestamp;
		//Ignore if montage is repeating.
		if (EndTimestamp < 0) continue;
		if (FormCharacterComponent->CalculateTimeSincePredictedTimestamp(EndTimestamp) > RecentMontageTimeout)
		{
			//Order is not retained.
			RecentMontages.RemoveAtSwap(i, 1, false);
		}
	}

	RecentMontages.Shrink();
}

void UFormAnimComponent::RecreateMontagesFromRecentData(UAnimInstance* Instance,
                                                        const TArray<FRecentMontageData>& InRecentMontages,
                                                        const UFormCharacterComponent* InFormCharacterComponent)
{
	if (!Instance) return;

	//Clear currently playing montages.
	Instance->Montage_Stop(0.f);

	for (const FRecentMontageData& MontageData : InRecentMontages)
	{
		//Skip if we've already interrupted this montage.
		if (MontageData.InterruptedTimestamp < 0) continue;
		if (MontageData.ExpectedEndTimestamp >= 0)
		{
			//Ignore if montage isn't looping and has theoretically finished playing.
			if (InFormCharacterComponent->CalculateTimeSincePredictedTimestamp(MontageData.ExpectedEndTimestamp) > 0) continue;
		}
		//Recreate montage with correct start point. We modulo the difference with the start timestamp if the montage is looping.
		const float TimeToStartMontageAt = FMath::Fmod(InFormCharacterComponent->CalculateTimeSincePredictedTimestamp(MontageData.StartTimestamp), MontageData.Montage->GetPlayLength());
		Instance->Montage_Play(MontageData.Montage, 1.f, EMontagePlayReturnType::MontageLength, TimeToStartMontageAt, false);
	}
}

void UFormAnimComponent::RollBackMontageData(TArray<FRecentMontageData>& InRecentMontages,
	const UFormCharacterComponent* InFormCharacterComponent)
{
	if (!InFormCharacterComponent) return;

	//Timestamp should be reset to right before the first replay tick at this point.
	
	for (int16 i = InRecentMontages.Num(); i >= 0; i--)
	{
		//Iterate backwards to remove elements that were created after the rollback timestamp.
		if (InFormCharacterComponent->CalculateTimeUntilPredictedTimestamp(InRecentMontages[i].StartTimestamp) > 0)
		{
			//Order is not retained.
			InRecentMontages.RemoveAtSwap(i, 1, false);
			continue;
		}
		//Remove interruptions if they were performed after the timestamp.
		if (InRecentMontages[i].InterruptedTimestamp >= 0 && InFormCharacterComponent->CalculateTimeUntilPredictedTimestamp(InRecentMontages[i].InterruptedTimestamp) > 0)
		{
			InRecentMontages[i].InterruptedTimestamp = -1;
		}
	}

	InRecentMontages.Shrink();
}

UAnimInstance* UFormAnimComponent::GetAnimInstanceChecked(const USkeletalMeshComponent* SkeletalMeshComponent) const
{
	if (!SkeletalMeshComponent)
	{
		UE_LOG(LogSfAudiovisual, Error,
		       TEXT("UFormAnimComponent tried to call montage on a missing USkeletalMeshComponent."));
		return nullptr;
	}
	if (UAnimInstance* Instance = SkeletalMeshComponent->GetAnimInstance())
	{
		return Instance;
	}
	UE_LOG(LogSfAudiovisual, Error, TEXT("UFormAnimComponent tried to call montage on a missing UAnimInstance."));
	return nullptr;
}
