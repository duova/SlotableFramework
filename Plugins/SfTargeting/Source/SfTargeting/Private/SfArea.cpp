﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "SfArea.h"

#include "Constituent.h"
#include "FormCoreComponent.h"

DEFINE_LOG_CATEGORY(LogSfTargeting);

ASfArea::ASfArea()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);
}

ASfArea* ASfArea::SpawnArea(UConstituent* Target, const TSubclassOf<ASfArea>& InClass,
	const FVector& InLocation, const FRotator& InRotation, const TArray<AActor*>& InActorsToIgnore,
	const TArray<FGameplayTag>& InTeamsToIgnore, const float TickInterval, const FAreaOverlap OnEnterEvent,
	const FAreaOverlap OnExitEvent, const FAreaOverlap OnTickEvent)
{
	ASfArea* Area = Cast<ASfArea>(Target->GetWorld()->SpawnActor(InClass, &InLocation, &InRotation));
	for (AActor* Actor : InActorsToIgnore)
	{
		Area->ActorsToIgnore.Add(Actor);
	}
	for (const FGameplayTag& Team : InTeamsToIgnore)
	{
		Area->TeamsToIgnore.Add(Team);
	}
	if (TickInterval <= 0.f)
	{
		Area->SetActorTickEnabled(false);
	}
	else
	{
		Area->SetActorTickInterval(TickInterval);
	}
	Area->OnEnter = OnEnterEvent;
	Area->OnExit = OnExitEvent;
	Area->OnTick = OnTickEvent;
	return Area;
}

void ASfArea::BeginPlay()
{
	Super::BeginPlay();
	for (UActorComponent* Component : GetComponents())
	{
		if (Component && Component->GetClass()->IsChildOf(UPrimitiveComponent::StaticClass()))
		{
			PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
			break;
		}
	}
	if (!PrimitiveComponent)
	{
		UE_LOG(LogSfTargeting, Error, TEXT("Spawned SfArea with class %s without a primitive component. Destroying."), *GetClass()->GetName());
		Destroy(true);
	}
	else
	{
		OnEnterOverlapDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ASfArea, OnEnterOverlap));
		OnExitOverlapDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ASfArea, OnExitOverlap));
		PrimitiveComponent->OnComponentBeginOverlap.Add(OnEnterOverlapDelegate);
		PrimitiveComponent->OnComponentEndOverlap.Add(OnExitOverlapDelegate);
		
		PrimitiveComponent->SetGenerateOverlapEvents(true);
	}
}

void ASfArea::OnEnterOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ActorsToIgnore.Contains(OtherActor)) return;
	if (UFormCoreComponent* FormCore = Cast<UFormCoreComponent>(OtherActor->GetComponentByClass(UFormCoreComponent::StaticClass())))
	{
		if (TeamsToIgnore.Contains(FormCore->GetTeam())) return;
	}
	OverlappingActors.Add(OtherActor);
	OnEnter.ExecuteIfBound(OtherActor, this, GetActorLocation());
}

void ASfArea::OnExitOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ActorsToIgnore.Contains(OtherActor)) return;
	if (UFormCoreComponent* FormCore = Cast<UFormCoreComponent>(OtherActor->GetComponentByClass(UFormCoreComponent::StaticClass())))
	{
		if (TeamsToIgnore.Contains(FormCore->GetTeam())) return;
	}
	OverlappingActors.Remove(OtherActor);
	OnExit.ExecuteIfBound(OtherActor, this, GetActorLocation());
}

void ASfArea::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!PrimitiveComponent) return;

	OverlappingActors.Remove(nullptr);
	for (AActor* Actor : OverlappingActors)
	{
		if (!Actor) continue;
		OnTick.ExecuteIfBound(Actor, this, GetActorLocation());
	}
}
