// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SfGameState.generated.h"

/**
 * AGameState with SF features. It is recommended that GameState for projects that use SF extend off this class.
 * The GameStateClass variable must be changed on the ASfGameMode if this is class is extended.
 */
UCLASS(Blueprintable)
class SFCORE_API ASfGameState : public AGameState
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetRawReplicatedServerWorldTime() const;
};
