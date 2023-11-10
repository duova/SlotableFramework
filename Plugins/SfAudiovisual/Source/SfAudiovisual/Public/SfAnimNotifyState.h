// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "SfAnimNotifyState.generated.h"

/**
 * Anim notify that is compatible with SF prediction.
 * Except for default implementations, this class can be extended with the SfNotify events implemented to do things
 * such as spawning actors and changing material properties. It is recommended that the default events are not used.
 * The logic must be stateless - i.e. it must be able to start at any TimeSinceStart given by SfNotifyBegin and
 * end whenever SfNotifyEnd runs. This is because prediction can cause early and delayed client side montage
 * execution.
 */
UCLASS(Abstract, Blueprintable)
class SFAUDIOVISUAL_API USfAnimNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual void InternalSfNotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference, const float TimeSinceStart);

	virtual void InternalSfNotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference, const float TimeSinceStart);

	UFUNCTION(BlueprintImplementableEvent, meta=(AutoCreateRefTerm="EventReference"))
	void SfNotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference, const float TimeSinceStart) const;

	UFUNCTION(BlueprintImplementableEvent, meta=(AutoCreateRefTerm="EventReference"))
	void SfNotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference, const float TimeSinceStart) const;

	UFUNCTION(BlueprintCallable)
	static AActor* SpawnActor(USkeletalMeshComponent* MeshComp, const TSubclassOf<AActor>& Class, const FVector Location, const FRotator Rotation);

protected:
	float GetTimeSinceNotifyStart(const FAnimNotifyEventReference& EventReference) const;
};
