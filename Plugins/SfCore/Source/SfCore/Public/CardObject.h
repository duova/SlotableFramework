// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CardObject.generated.h"

/**
 * Card objects are definitions for cards, but may be optionally spawned on the client for the lifetime of the card. They
 * are blueprintable, but should be expected to be initialized or deinitialized at any time.
 */
UCLASS(Blueprintable)
class SFCORE_API UCardObject : public UObject
{
	GENERATED_BODY()

public:
	UCardObject();

	UPROPERTY(BlueprintReadOnly)
	class UInventory* OwningInventory = nullptr;

	UFUNCTION(BlueprintImplementableEvent)
	void Initialize();

	UFUNCTION(BlueprintImplementableEvent)
	void Deinitialize();

	//Should this card object be spawned on the client when the form has the card?
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Card Object")
	bool bSpawnCardObjectOnClient = false;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Card Object")
	bool bUseLifetime = false;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (ClampMin = 0, ClampMax = 999999), Category = "Card Object")
	float DefaultLifetimeInSeconds = 0;
	
	uint16 OwnerConstituentInstanceId = 0;

	//If true, this applies the speed modifier variables to the movement variables of the FormCharacterComponent.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Card Object")
	bool bUseMovementSpeedModifiers = false;

	//Initial +- to movement speed.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Card Object", meta = (ClampMin = 0, ClampMax = 999999))
	float AdditiveMovementSpeedModifier = 0;

	//Movement speed is multiplied by (sums of these values + 1).
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Card Object", meta = (ClampMin = 0, ClampMax = 999999))
	float AdditiveMultiplicativeMovementSpeedModifier = 0;

	//Directly multiplies movement speed by this value, after additive multiplicative. Multiple values multiply.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Card Object", meta = (ClampMin = 0, ClampMax = 999999))
	float TrueMultiplicativeMovementSpeedModifier = 1;

	//+- to movement speed after all other modifications.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Card Object", meta = (ClampMin = 0, ClampMax = 999999))
	float FlatAdditiveMovementSpeedModifier = 0;
};
