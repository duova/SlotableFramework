// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FormCharacterComponent.generated.h"

/*
 * The FormCharacterComponent is responsible for the movement and client prediction of a form.
 * The form must be an ACharacter. This component must be replicated.
 * The component should be used for all forms that may be player controlled or require CharacterMovementComponent features.
 * AI controllers and player controllers should write inputs to this component.
 * This component should not be used if the form will never be player controlled and does not need character movement.
 * In that case, the controllers should directly call functions on FormBaseComponent instead of writing inputs.
 * This component extends the CharacterMovementComponent to use its prediction logic to drive prediction for slotables.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SFCORE_API UFormCharacterComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UFormCharacterComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
