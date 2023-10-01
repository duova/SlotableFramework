// Fill out your copyright notice in the Description page of Project Settings.


#include "SfAnimNotifyState.h"

void USfAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	InternalSfNotifyBegin(MeshComp, Animation, TotalDuration, EventReference, GetTimeSinceNotifyStart(EventReference));
}

void USfAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	InternalSfNotifyEnd(MeshComp, Animation, EventReference, GetTimeSinceNotifyStart(EventReference));
}

void USfAnimNotifyState::InternalSfNotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference, const float TimeSinceStart)
{
	SfNotifyBegin(MeshComp, Animation, TotalDuration, EventReference, TimeSinceStart);
}

void USfAnimNotifyState::InternalSfNotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference, const float TimeSinceStart)
{
	SfNotifyEnd(MeshComp, Animation, EventReference, TimeSinceStart);
}

float USfAnimNotifyState::GetTimeSinceNotifyStart(const FAnimNotifyEventReference& EventReference) const
{
	const float Time = EventReference.GetNotify()->GetTime(EAnimLinkMethod::Relative);
	checkf(Time != -1.f, TEXT("Notify GetTime could not get relative time."));
	return FMath::Max(Time, 0.f);
}
