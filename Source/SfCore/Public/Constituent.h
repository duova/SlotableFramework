// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfObject.h"
#include "Slotable.h"
#include "Constituent.generated.h"

/**
 * A blueprint scripted component of a slotable.
 */
UCLASS(Blueprintable)
class SFCORE_API UConstituent : public USfObject
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere)
	USlotable* OwningSlotable;
	
	void Initialize();

	void Deinitialize();
};
