// Fill out your copyright notice in the Description page of Project Settings.


#include "FormPawn.h"

#include "FormCoreComponent.h"
#include "RelevancyArea.h"


// Sets default values
AFormPawn::AFormPawn()
{
	FormCore = CreateDefaultSubobject<UFormCoreComponent>(FormCoreComponentName);
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	bReplicateUsingRegisteredSubObjectList = true;
}

bool AFormPawn::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
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
