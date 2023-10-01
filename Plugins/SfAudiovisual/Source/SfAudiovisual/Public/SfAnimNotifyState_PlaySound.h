// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfAnimNotifyState.h"
#include "SfAnimNotifyState_PlaySound.generated.h"

/**
 * SF implementation of AnimNotify_PlaySound.
 */
UCLASS(const, hidecategories=Object, collapsecategories, Config = Game, meta=(DisplayName="SF Play Sound"))
class SFAUDIOVISUAL_API USfAnimNotifyState_PlaySound : public USfAnimNotifyState
{
	GENERATED_BODY()

public:
	
	USfAnimNotifyState_PlaySound();

	// Begin USfAnimNotifyState interface
	virtual FString GetNotifyName_Implementation() const override;
	virtual void InternalSfNotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference, const float TimeSinceStart) override;
	virtual void InternalSfNotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference, const float TimeSinceStart) override;
#if WITH_EDITOR
	virtual void ValidateAssociatedAssets() override;
#endif
	// End USfAnimNotifyState interface

	// Sound to Play
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify", meta=(ExposeOnSpawn = true))
	TObjectPtr<USoundBase> Sound;

	// Volume Multiplier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify", meta=(ExposeOnSpawn = true))
	float VolumeMultiplier;

	// Pitch Multiplier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify", meta=(ExposeOnSpawn = true))
	float PitchMultiplier;

	// If this sound should follow its owner
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	uint32 bFollow:1;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Config, EditAnywhere, Category = "AnimNotify")
	uint32 bPreviewIgnoreAttenuation:1;
#endif

	// Socket or bone name to attach sound to
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify", meta=(EditCondition="bFollow", ExposeOnSpawn = true))
	FName AttachName;

	UPROPERTY()
	UAudioComponent* CurrentAudioComponent;
};
