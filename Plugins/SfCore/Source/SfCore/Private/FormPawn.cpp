// Fill out your copyright notice in the Description page of Project Settings.


#include "FormPawn.h"

#include "FormCoreComponent.h"


// Sets default values
AFormPawn::AFormPawn()
{
	FormCore = CreateDefaultSubobject<UFormCoreComponent>(FormCoreComponentName);
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	bReplicateUsingRegisteredSubObjectList = true;
}

void AFormPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
}
