// Fill out your copyright notice in the Description page of Project Settings.


#include "SfAnimNotifyState_PlaySound.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/UObjectToken.h"

//Implementation here is mostly from AnimNotify_PlaySound.

USfAnimNotifyState_PlaySound::USfAnimNotifyState_PlaySound() : Super(), bFollow(0)
{
	VolumeMultiplier = 1.f;
	PitchMultiplier = 1.f;

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(196, 142, 255, 255);
	bPreviewIgnoreAttenuation = false;
#endif // WITH_EDITORONLY_DATA
}

FString USfAnimNotifyState_PlaySound::GetNotifyName_Implementation() const
{
	if (Sound)
	{
		return Sound->GetName();
	}
	else
	{
		return Super::GetNotifyName_Implementation();
	}
}

void USfAnimNotifyState_PlaySound::InternalSfNotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                         float TotalDuration,
                                                         const FAnimNotifyEventReference& EventReference,
                                                         const float TimeSinceStart)
{
	// Don't call super to avoid call back in to blueprints
	if (Sound && MeshComp)
	{
		if (!Sound->IsOneShot())
		{
			UE_LOG(LogAudio, Warning,
			       TEXT(
				       "SF PlaySound notify: Anim %s tried to play a sound asset which is not a one-shot: '%s'. Spawning suppressed."
			       ), *GetNameSafe(Animation), *GetNameSafe(Sound));
			return;
		}

#if WITH_EDITORONLY_DATA
		UWorld* World = MeshComp->GetWorld();
		if (bPreviewIgnoreAttenuation && World && World->WorldType == EWorldType::EditorPreview)
		{
			CurrentAudioComponent = UGameplayStatics::SpawnSound2D(World, Sound, VolumeMultiplier, PitchMultiplier,
			                                                       TimeSinceStart);
		}
		else
#endif
		{
			if (bFollow)
			{
				CurrentAudioComponent = UGameplayStatics::SpawnSoundAttached(
					Sound, MeshComp, AttachName, FVector(ForceInit), EAttachLocation::SnapToTarget, false,
					VolumeMultiplier, PitchMultiplier, TimeSinceStart);
			}
			else
			{
				CurrentAudioComponent = UGameplayStatics::SpawnSoundAtLocation(
					MeshComp->GetWorld(), Sound, MeshComp->GetComponentLocation(), FRotator::ZeroRotator,
					VolumeMultiplier, PitchMultiplier, TimeSinceStart);
			}
		}
	}
}

void USfAnimNotifyState_PlaySound::InternalSfNotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                       const FAnimNotifyEventReference& EventReference,
                                                       const float TimeSinceStart)
{
	// Don't call super to avoid call back in to blueprints
	if (!CurrentAudioComponent) return;
	CurrentAudioComponent->Stop();
	CurrentAudioComponent = nullptr;
}

#if WITH_EDITOR
void USfAnimNotifyState_PlaySound::ValidateAssociatedAssets()
{
	static const FName NAME_AssetCheck("AssetCheck");

	if (Sound != nullptr && !Sound->IsOneShot())
	{
		UObject* ContainingAsset = GetContainingAsset();

		FMessageLog AssetCheckLog(NAME_AssetCheck);

		const FText MessageLooping = FText::Format(
			NSLOCTEXT("AnimNotify", "Sound_ShouldNotLoop",
			          "Sound {0} used in anim notify for asset {1} is set to looping, but the slot is a one-shot (it won't be played to avoid leaking an instance per notify)."),
			FText::AsCultureInvariant(Sound->GetPathName()),
			FText::AsCultureInvariant(ContainingAsset->GetPathName()));
		AssetCheckLog.Warning()
		             ->AddToken(FUObjectToken::Create(ContainingAsset))
		             ->AddToken(FTextToken::Create(MessageLooping));

		if (GIsEditor)
		{
			AssetCheckLog.Notify(MessageLooping, EMessageSeverity::Warning, /*bForce=*/ true);
		}
	}
}
#endif
