// Fill out your copyright notice in the Description page of Project Settings.


#include "FormCharacter.h"

#include "FormCharacterComponent.h"

AFormCharacter::AFormCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UFormCharacterComponent>(CharacterMovementComponentName))
{
	FormCore = CreateDefaultSubobject<UFormCoreComponent>(FormCoreComponentName);
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	bReplicateUsingRegisteredSubObjectList = true;
}
