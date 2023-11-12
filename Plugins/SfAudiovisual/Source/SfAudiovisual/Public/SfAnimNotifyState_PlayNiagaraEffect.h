// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraSystem.h"
#include "SfAnimNotifyState.h"
#include "SfAnimNotifyState_PlayNiagaraEffect.generated.h"

/**
 * SF implementation of AnimNotify_PlayNiagaraEffect.
 */
UCLASS(const, hidecategories = Object, collapsecategories, meta = (DisplayName = "SF Play Niagara Particle Effect"))
class SFAUDIOVISUAL_API USfAnimNotifyState_PlayNiagaraEffect : public USfAnimNotifyState
{
	GENERATED_BODY()

public:
	
	USfAnimNotifyState_PlayNiagaraEffect();
	
	// Begin UObject interface
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	// End UObject interface

	// Begin USfAnimNotifyState interface
	virtual FString GetNotifyName_Implementation() const override;
	virtual void InternalSfNotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference, const float TimeSinceStart) override;
	virtual void InternalSfNotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference, const float TimeSinceStart) override;
#if WITH_EDITOR
	virtual void ValidateAssociatedAssets() override;
#endif
	// End USfAnimNotifyState interface

	// Niagara System to Spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify", meta = (DisplayName = "Niagara System"))
	TObjectPtr<UNiagaraSystem> Template;

	// Location offset from the socket
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	FVector LocationOffset;

	// Rotation offset from socket
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	FRotator RotationOffset;

	// Scale to spawn the Niagara system at
	UPROPERTY(EditAnywhere, Category = "AnimNotify")
	FVector Scale;

	// Whether or not we are in absolute scale mode
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "AnimNotify")
	bool bAbsoluteScale;

	// Return FXSystemComponent created from SpawnEffect
	UFUNCTION(BlueprintCallable, Category = "AnimNotify")
	UFXSystemComponent* GetSpawnedEffect();

protected:

	//FXSystem Pointer to Spawned Effect called from Notify.
	UPROPERTY()
	UFXSystemComponent* SpawnedEffect;

	// Cached version of the Rotation Offset already in Quat form
	FQuat RotationOffsetQuat;

	// Spawns the NiagaraSystemComponent. Called from InternalSfNotifyBegin.
	virtual UFXSystemComponent* SpawnEffect(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const float TimeSinceStart);


public:

	// Should attach to the bone/socket
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	uint32 Attached : 1; 	//~ Does not follow coding standard due to redirection from BP

	// SocketName to attach to
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify", meta = (AnimNotifyBoneName = "true"))
	FName SocketName;
};
