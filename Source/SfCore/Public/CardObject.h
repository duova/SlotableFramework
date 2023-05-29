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
	
	virtual void BeginDestroy() override;

	UPROPERTY(BlueprintReadOnly)
	class UInventory* OwningInventory;

	void Initialize();

	void Deinitialize();

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	uint8 bSpawnCardObjectOnClient:1;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	uint8 bUseLifetime:1;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float DefaultLifetime;
	
	uint16 OwnerConstituentInstanceId;

	//Initial +- to movement speed.
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float AdditiveMovementSpeedModifier;

	//Movement speed is multiplied by (sums of these values + 1).
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float AdditiveMultiplicativeMovementSpeedModifier;

	//Directly multiplies movement speed by this value, after additive multiplicative. Multiple values multiply.
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float TrueMultiplicativeMovementSpeedModifier;

	//+- to movement speed after all other modifications.
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float FlatAdditiveMovementSpeedModifier;
};
