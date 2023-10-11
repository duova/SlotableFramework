// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SfAudiovisualCommon.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSfAudiovisual, Log, All);

UENUM(BlueprintType)
enum class EPerspective : uint8
{
	FirstPerson,
	ThirdPerson
};