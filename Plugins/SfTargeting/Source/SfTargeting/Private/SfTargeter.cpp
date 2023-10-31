// Fill out your copyright notice in the Description page of Project Settings.


#include "SfTargeter.h"

#include "Constituent.h"
#include "FormCoreComponent.h"

DEFINE_LOG_CATEGORY(LogSfTargeter);

ASfTargeter::ASfTargeter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);
}

ASfTargeter* ASfTargeter::SpawnTargeter(UConstituent* Target, const TSubclassOf<ASfTargeter>& InClass,
	const FVector& InLocation, const FRotator& InRotation, const TArray<AActor*>& InActorsToIgnore,
	const TArray<FGameplayTag>& InTeamsToIgnore, const float TickInterval, FTargeterOverlap& OutOnEnter,
	FTargeterOverlap& OutOnExit, FTargeterOverlap& OutOnTick)
{
	ASfTargeter* Targeter = Cast<ASfTargeter>(Target->GetWorld()->SpawnActor(InClass, &InLocation, &InRotation));
	for (AActor* Actor : InActorsToIgnore)
	{
		Targeter->ActorsToIgnore.Add(Actor);
	}
	for (const FGameplayTag& Team : InTeamsToIgnore)
	{
		Targeter->TeamsToIgnore.Add(Team);
	}
	Targeter->SetActorTickInterval(TickInterval);
	Targeter->OnEnter = OutOnEnter;
	Targeter->OnExit = OutOnExit;
	Targeter->OnTick = OutOnTick;
	return Targeter;
}

void ASfTargeter::BeginPlay()
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
		UE_LOG(LogSfTargeter, Error, TEXT("Spawned SfTargeter with class %s without a primitive component. Destroying."), *GetClass()->GetName());
		Destroy(true);
	}
	else
	{
		OnEnterOverlapDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ASfTargeter, OnEnterOverlap));
		OnExitOverlapDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ASfTargeter, OnExitOverlap));
		PrimitiveComponent->OnComponentBeginOverlap.Add(OnEnterOverlapDelegate);
		PrimitiveComponent->OnComponentEndOverlap.Add(OnExitOverlapDelegate);
		
		PrimitiveComponent->SetGenerateOverlapEvents(true);
	}
}

void ASfTargeter::OnEnterOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ActorsToIgnore.Contains(OtherActor)) return;
	if (UFormCoreComponent* FormCore = Cast<UFormCoreComponent>(OtherActor->GetComponentByClass(UFormCoreComponent::StaticClass())))
	{
		if (TeamsToIgnore.Contains(FormCore->GetTeam())) return;
	}
	OverlappingActors.Add(OtherActor);
	OnEnter.ExecuteIfBound(OtherActor);
}

void ASfTargeter::OnExitOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ActorsToIgnore.Contains(OtherActor)) return;
	if (UFormCoreComponent* FormCore = Cast<UFormCoreComponent>(OtherActor->GetComponentByClass(UFormCoreComponent::StaticClass())))
	{
		if (TeamsToIgnore.Contains(FormCore->GetTeam())) return;
	}
	OverlappingActors.Remove(OtherActor);
	OnExit.ExecuteIfBound(OtherActor);
}

void ASfTargeter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!PrimitiveComponent) return;

	OverlappingActors.Remove(nullptr);
	for (AActor* Actor : OverlappingActors)
	{
		if (!Actor) continue;
		OnTick.ExecuteIfBound(Actor);
	}
}

