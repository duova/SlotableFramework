// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfObject.h"
#include "UObject/Object.h"
#include "SfTest.generated.h"

/**
 * Base class for a test that will be found by the test runner and executed assuming conditions are matched.
 */
UCLASS()
class SFTESTS_API USfTest : public USfObject
{
	GENERATED_BODY()

public:
	void Execute();
};
