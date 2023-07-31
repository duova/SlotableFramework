// Fill out your copyright notice in the Description page of Project Settings.


#include "FormActor.h"

#include "FormCoreComponent.h"


// Sets default values
AFormActor::AFormActor()
{
	FormCore = CreateDefaultSubobject<UFormCoreComponent>(FormCoreComponentName);
	bReplicates = true;
	CreateDefaultSubobject(FName("FormCore"), UFormCoreComponent::StaticClass(), UFormCoreComponent::StaticClass(), true, false);
}

