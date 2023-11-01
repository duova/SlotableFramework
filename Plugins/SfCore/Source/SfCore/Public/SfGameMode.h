// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FormCoreComponent.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameMode.h"
#include "SfGameMode.generated.h"

//Since nested containers are not supported.
USTRUCT()
struct SFCORE_API FFormGroup
{
	GENERATED_BODY()

	UPROPERTY()
	TSet<UFormCoreComponent*> Members;

	FFormGroup();
};

/**
 * AGameMode with SF features. It is recommended that GameModes for projects that use SF extend off this class.
 */
UCLASS()
class SFCORE_API ASfGameMode : public AGameMode
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<AActor*> GetActorsInTeam(const FGameplayTag InTeam);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<UFormCoreComponent*> GetFormCoresInTeam(const FGameplayTag InTeam);
	
	void AddToTeam(UFormCoreComponent* InFormCore, const FGameplayTag& InTeam);

	bool RemoveFromTeam(const UFormCoreComponent* InFormCore, const FGameplayTag& InTeam);

private:

	UPROPERTY()
	TMap<FGameplayTag, FFormGroup> TeamRegistry;
};
