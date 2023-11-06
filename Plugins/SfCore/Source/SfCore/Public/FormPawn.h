// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FormPawn.generated.h"

class ARelevancyArea;
class UFormCoreComponent;
/**
 * APawn with a FormCoreComponent.
 * Default pawn for all forms that cannot move organically and are not player controlled.
 */
UCLASS(Blueprintable)
class SFCORE_API AFormPawn : public APawn
{
	GENERATED_BODY()

public:
	AFormPawn();

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	UFormCoreComponent* FormCore;

	inline static FName FormCoreComponentName = "FormCoreComp";

	//Form relevancy is based on ARelevancyArea.
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;

	UPROPERTY()
	TSet<ARelevancyArea*> RelevancyAreas;
};
