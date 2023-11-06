// Fill out your copyright notice in the Description page of Project Settings.


#include "SfArea.h"

#include "Constituent.h"
#include "FormCoreComponent.h"
#include "SfTargetingLibrary.h"

ASfArea::ASfArea()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(true);
}

ASfArea* ASfArea::Server_SpawnSfArea(UConstituent* Target, const TSubclassOf<ASfArea>& InClass,
                            const FVector& InLocation, const FRotator& InRotation,
                            const TArray<AActor*>& InActorsToIgnore,
                            const TArray<FGameplayTag>& InTeamsToIgnore,
                            const float InTickInterval, const FAreaOverlap OnEnterEvent,
                            const FAreaOverlap OnExitEvent, const FAreaOverlap OnTickEvent,
                            const bool bInOnlyTargetForms)
{
	if (InClass->HasAnyClassFlags(CLASS_Abstract))
	{
		UE_LOG(LogSfTargeting, Error, TEXT("Tried to spawn abstract ASfArea class."));
		return nullptr;
	}
	
	ASfArea* Area = Cast<ASfArea>(Target->GetWorld()->SpawnActor(InClass, &InLocation, &InRotation));
	for (AActor* Actor : InActorsToIgnore)
	{
		Area->ActorsToIgnore.Add(Actor);
	}
	for (const FGameplayTag& Team : InTeamsToIgnore)
	{
		Area->TeamsToIgnore.Add(Team);
	}
	if (InTickInterval <= 0.f)
	{
		Area->SetActorTickEnabled(false);
	}
	else
	{
		Area->SetActorTickInterval(InTickInterval);
	}
	Area->bOnlyTargetForms = bInOnlyTargetForms;
	if (OnEnterEvent.IsBound())
	{
		Area->OnEnter = OnEnterEvent;
	}
	if (OnExitEvent.IsBound())
	{
		Area->OnExit = OnExitEvent;
	}
	if (OnTickEvent.IsBound())
	{
		Area->OnTick = OnTickEvent;
	}
	return Area;
}

TArray<AActor*> ASfArea::GetActorsInArea() const
{
	return OverlappingActors.Array();
}

void ASfArea::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) return;
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
		UE_LOG(LogSfTargeting, Error, TEXT("Spawned ASfArea with class %s without a primitive component. Destroying."),
		       *GetClass()->GetName());
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
                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                             const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	if (ActorsToIgnore.Contains(OtherActor)) return;
	if (UFormCoreComponent* FormCore = Cast<UFormCoreComponent>(
		OtherActor->GetComponentByClass(UFormCoreComponent::StaticClass())))
	{
		if (TeamsToIgnore.Contains(FormCore->GetTeam())) return;
	}
	else if (bOnlyTargetForms) return;
	OverlappingActors.Add(OtherActor);
	OnEnter.ExecuteIfBound(OtherActor, this, GetActorLocation());
}

void ASfArea::OnExitOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                            UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority()) return;
	if (ActorsToIgnore.Contains(OtherActor)) return;
	if (const UFormCoreComponent* FormCore = Cast<UFormCoreComponent>(
		OtherActor->GetComponentByClass(UFormCoreComponent::StaticClass())))
	{
		if (TeamsToIgnore.Contains(FormCore->GetTeam())) return;
	}
	else if (bOnlyTargetForms) return;
	OverlappingActors.Remove(OtherActor);
	OnExit.ExecuteIfBound(OtherActor, this, GetActorLocation());
}

void ASfArea::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!HasAuthority()) return;

	if (!PrimitiveComponent) return;

	OverlappingActors.Remove(nullptr);
	for (AActor* Actor : OverlappingActors)
	{
		if (!Actor) continue;
		OnTick.ExecuteIfBound(Actor, this, GetActorLocation());
	}
}
