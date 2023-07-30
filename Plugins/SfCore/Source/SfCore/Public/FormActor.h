// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FormActor.generated.h"

class UFormCoreComponent;
/**
 * AActor with a FormCoreComponent.
 * Default actor for all forms that cannot move organically and are not player controlled.
 */
UCLASS(Blueprintable)
class SFCORE_API AFormActor : public AActor
{
	GENERATED_BODY()

public:
	AFormActor();

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	UFormCoreComponent* FormCore;

	inline static FName FormCoreComponentName = "FormCoreComp";
};
