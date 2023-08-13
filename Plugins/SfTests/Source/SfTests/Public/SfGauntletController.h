// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GauntletTestController.h"
#include "SfGauntletController.generated.h"

class ASfTestRunner;
/**
 * Main test controller for the slotable framework.
 */
UCLASS()
class SFTESTS_API USfGauntletController : public UGauntletTestController
{
	GENERATED_BODY()

public:
	virtual void OnPostMapChange(UWorld* World) override;

	UPROPERTY()
	ASfTestRunner* ServerTestRunner;

	static void SfEndSession(const bool bPassed);
};
