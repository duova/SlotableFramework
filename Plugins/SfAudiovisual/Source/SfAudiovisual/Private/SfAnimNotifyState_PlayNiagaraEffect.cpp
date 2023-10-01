// Fill out your copyright notice in the Description page of Project Settings.


#include "SfAnimNotifyState_PlayNiagaraEffect.h"

#if WITH_EDITOR
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#endif

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SfAnimNotifyState_PlayNiagaraEffect)

//Implementation here is mostly from AnimNotify_PlayNiagaraEffect.

USfAnimNotifyState_PlayNiagaraEffect::USfAnimNotifyState_PlayNiagaraEffect()
	: Super(), SpawnedEffect(nullptr)
{
	Attached = true;
	Scale = FVector(1.f);
	bAbsoluteScale = false;

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(192, 255, 99, 255);
#endif // WITH_EDITORONLY_DATA
}

void USfAnimNotifyState_PlayNiagaraEffect::PostLoad()
{
	Super::PostLoad();

	RotationOffsetQuat = FQuat(RotationOffset);
}

#if WITH_EDITOR
void USfAnimNotifyState_PlayNiagaraEffect::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.MemberProperty && PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USfAnimNotifyState_PlayNiagaraEffect, RotationOffset))
	{
		RotationOffsetQuat = FQuat(RotationOffset);
	}
}

void USfAnimNotifyState_PlayNiagaraEffect::ValidateAssociatedAssets()
{
	static const FName NAME_AssetCheck("AssetCheck");

	if ((Template != nullptr) && (Template->IsLooping()))
	{
		UObject* ContainingAsset = GetContainingAsset();

		FMessageLog AssetCheckLog(NAME_AssetCheck);

		const FText MessageLooping = FText::Format(
			NSLOCTEXT("AnimNotify", "NiagaraSystem_ShouldNotLoop", "Niagara system {0} used in anim notify for asset {1} is set to looping, but the slot is a one-shot (it won't be played to avoid leaking a component per notify)."),
			FText::AsCultureInvariant(Template->GetPathName()),
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

void USfAnimNotifyState_PlayNiagaraEffect::InternalSfNotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference, const float TimeSinceStart)
{
	//Store the spawned effect in a protected variable
	SpawnedEffect = SpawnEffect(MeshComp, Animation, TimeSinceStart);
	
	//Call to BP to allows setting of Niagara User Variables
	Super::InternalSfNotifyBegin(MeshComp, Animation, TotalDuration, EventReference, TimeSinceStart);
}

void USfAnimNotifyState_PlayNiagaraEffect::InternalSfNotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference, const float TimeSinceStart)
{
	if (!SpawnedEffect) return;
	SpawnedEffect->DestroyComponent();
}

FString USfAnimNotifyState_PlayNiagaraEffect::GetNotifyName_Implementation() const
{
	if (Template)
	{
		return Template->GetName();
	}
	else
	{
		return Super::GetNotifyName_Implementation();
	}
}

UFXSystemComponent* USfAnimNotifyState_PlayNiagaraEffect::SpawnEffect(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const float TimeSinceStart)
{
	UNiagaraComponent* ReturnComp = nullptr;

	if (Template)
	{
		if (Template->IsLooping())
		{
			return ReturnComp;
		}

		if (Attached)
		{
			ReturnComp = UNiagaraFunctionLibrary::SpawnSystemAttached(Template, MeshComp, SocketName, LocationOffset, RotationOffset, EAttachLocation::KeepRelativeOffset, true);
		}
		else
		{
			const FTransform MeshTransform = MeshComp->GetSocketTransform(SocketName);
			ReturnComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(MeshComp->GetWorld(), Template, MeshTransform.TransformPosition(LocationOffset), (MeshTransform.GetRotation() * RotationOffsetQuat).Rotator(), FVector(1.0f),true);
		}

		if (ReturnComp != nullptr)
		{
			ReturnComp->SetUsingAbsoluteScale(bAbsoluteScale);
			ReturnComp->SetRelativeScale3D_Direct(Scale);
			if (TimeSinceStart > 0.02)
			{
				//Simulate theoretical time passed in one tick.
				ReturnComp->AdvanceSimulation(1, TimeSinceStart);
			}
		}
	}

	return ReturnComp;
}

UFXSystemComponent* USfAnimNotifyState_PlayNiagaraEffect::GetSpawnedEffect()
{
	return SpawnedEffect;
}