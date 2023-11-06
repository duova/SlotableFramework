// Fill out your copyright notice in the Description page of Project Settings.


#include "RelevancyArea.h"

#include "FormCharacter.h"
#include "FormPawn.h"
#include "Components/ShapeComponent.h"

ARelevancyArea::ARelevancyArea()
{
	PrimaryActorTick.bCanEverTick = false;
	GetCollisionComponent()->SetGenerateOverlapEvents(true);
}

bool ARelevancyArea::Contains(const AActor* Actor) const
{
	return ActorsInArea.Contains(Actor);
}

void ARelevancyArea::BeginPlay()
{
	Super::BeginPlay();

	OnEnterOverlapDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ARelevancyArea, OnEnterOverlap));
	OnExitOverlapDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ARelevancyArea, OnExitOverlap));
	GetCollisionComponent()->OnComponentBeginOverlap.Add(OnEnterOverlapDelegate);
	GetCollisionComponent()->OnComponentEndOverlap.Add(OnExitOverlapDelegate);
}

void ARelevancyArea::OnEnterOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//Only deal with forms.
	if (AFormPawn* FormPawn = Cast<AFormPawn>(OtherActor))
	{
		FormPawn->RelevancyAreas.Add(this);
		ActorsInArea.Add(OtherActor);
	}
	if (AFormCharacter* FormCharacter = Cast<AFormCharacter>(OtherActor))
	{
		FormCharacter->RelevancyAreas.Add(this);
		ActorsInArea.Add(OtherActor);
	}
}

void ARelevancyArea::OnExitOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	//Only deal with forms.
	if (AFormPawn* FormPawn = Cast<AFormPawn>(OtherActor))
	{
		FormPawn->RelevancyAreas.Remove(this);
		ActorsInArea.Remove(OtherActor);
	}
	if (AFormCharacter* FormCharacter = Cast<AFormCharacter>(OtherActor))
	{
		FormCharacter->RelevancyAreas.Remove(this);
		ActorsInArea.Remove(OtherActor);
	}
}
