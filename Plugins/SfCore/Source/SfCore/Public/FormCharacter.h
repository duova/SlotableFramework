// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FormCharacter.generated.h"

class UFormCoreComponent;
/**
 * ACharacter using the FormCharacterComponent instead of the CharacterMovementComponent with a FormCoreComponent.
 * Default actor for all player controlled or moving forms.
 */
UCLASS(Blueprintable)
class SFCORE_API AFormCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFormCharacter(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	UFormCoreComponent* FormCore;

	inline static FName FormCoreComponentName = "FormCoreComp";
};
