// Fill out your copyright notice in the Description page of Project Settings.


#include "FormCharacter.h"

#include "FormCharacterComponent.h"
#include "RelevancyArea.h"

AFormCharacter::AFormCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UFormCharacterComponent>(CharacterMovementComponentName))
{
	FormCore = CreateDefaultSubobject<UFormCoreComponent>(FormCoreComponentName);
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	bReplicateUsingRegisteredSubObjectList = true;
}

void AFormCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	Cast<UFormCharacterComponent>(GetCharacterMovement())->SetupPlayerInputComponent(PlayerInputComponent);
}

bool AFormCharacter::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget,
	const FVector& SrcLocation) const
{
	if (bAlwaysRelevant) return true;
	for (const ARelevancyArea* Area : RelevancyAreas)
	{
		if (Area->Contains(ViewTarget)) return true;
		for (const ARelevancyArea* LinkedArea : Area->VisibleFrom)
		{
			if (LinkedArea->Contains(ViewTarget)) return true;
		}
	}
	return false;
}
