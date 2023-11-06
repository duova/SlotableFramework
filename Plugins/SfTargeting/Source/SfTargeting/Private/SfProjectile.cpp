// Fill out your copyright notice in the Description page of Project Settings.


#include "SfProjectile.h"

#include "FormCoreComponent.h"
#include "SfTargetingLibrary.h"
#include "Net/UnrealNetwork.h"

ASfProjectile::ASfProjectile(): ConstituentOwner(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(true);
	if (!CollisionComponent)
	{
		CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
		CollisionComponent->InitSphereRadius(15.0f);
		CollisionComponent->SetGenerateOverlapEvents(true);
		RootComponent = CollisionComponent;
	}
	if (!ProjectileMovementComponent)
	{
		ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(
			TEXT("ProjectileMovementComponent"));
		ProjectileMovementComponent->bShouldBounce = false;
	}
}

void ASfProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(ASfProjectile, ConstituentOwner, DefaultParams);
}

ASfProjectile* ASfProjectile::Server_SpawnSfProjectile(UConstituent* Target,
                                                       const TSubclassOf<ASfProjectile>& InClass,
                                                       const FVector& InOrigin, const FVector& InDirection,
                                                       const float InVelocity, const FSfProjectileParams& InParams,
                                                       const FProjectileOverlap OnOverlap)
{
	if (InClass->HasAnyClassFlags(CLASS_Abstract))
	{
		UE_LOG(LogSfTargeting, Error, TEXT("Tried to spawn abstract ASfProjectile class."));
		return nullptr;
	}

	//We always spawn the projectile at the server origin as on the client we only interpolate the visual.
	const FRotator Rot = InDirection.Rotation();
	ASfProjectile* Projectile = Cast<ASfProjectile>(Target->GetWorld()->SpawnActor(InClass, &InOrigin, &Rot));

	Projectile->ConstituentOwner = Target;
	MARK_PROPERTY_DIRTY_FROM_NAME(ASfProjectile, ConstituentOwner, Projectile);

	Projectile->OverlapEvent = OnOverlap;
	Projectile->Params = InParams;

	//Launch projectile.
	Projectile->ProjectileMovementComponent->Velocity = InDirection.GetSafeNormal() * InVelocity;

	return Projectile;
}

void ASfProjectile::BeginPlay()
{
	Super::BeginPlay();
	for (UActorComponent* Component : GetComponents())
	{
		if (Component && Component->GetClass()->IsChildOf(UMeshComponent::StaticClass()))
		{
			MeshComponent = Cast<UMeshComponent>(Component);
			break;
		}
	}
	if (!MeshComponent)
	{
		UE_LOG(LogSfTargeting, Error, TEXT("Spawned ASfProjectile with class %s without a mesh component. Destroying."),
		       *GetClass()->GetName());
		Destroy(true);
	}

	MeshComponent->OnComponentBeginOverlap.Add(InternalOverlap);
	InternalOverlap.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ASfProjectile, Overlap));
}

void ASfProjectile::Overlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//Check against params.
	if (Params.ActorsToIgnore.Contains(OtherActor)) return;
	if (const UFormCoreComponent* FormCore = Cast<UFormCoreComponent>(OtherActor->GetComponentByClass(UFormCoreComponent::StaticClass())))
	{
		if (Params.TeamsToIgnore.Contains(FormCore->GetTeam())) return;
		if (Params.TeamsToPassthrough.Contains(FormCore->GetTeam()))
		{
			OverlapEvent.ExecuteIfBound(OtherActor, OtherComp, this, GetActorLocation(), ProjectileMovementComponent->Velocity, true);
			return;
		}
	}
	if (Params.ActorsToPassthrough.Contains(OtherActor))
	{
		OverlapEvent.ExecuteIfBound(OtherActor, OtherComp, this, GetActorLocation(), ProjectileMovementComponent->Velocity, true);
		return;
	}
	//If not ignoring or passthrough, we remove the projectile after calling overlap event.
	OverlapEvent.ExecuteIfBound(OtherActor, OtherComp, this, GetActorLocation(), ProjectileMovementComponent->Velocity, false);
	Destroy(true);
}

void ASfProjectile::OnRep_ConstituentOwner()
{
	InternalClientOffset = ConstituentOwner->GetFormCoreComponent()->IsFirstPerson() ? FpOffset : TpOffset;
	MeshComponent->SetRelativeLocation(InternalClientOffset);
	TimeSinceLaunch = 0.01;
}

void ASfProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority()) return;
	//Wait for constituent owner to replicate before time starts ticking.
	if (TimeSinceLaunch > 0)
	{
		TimeSinceLaunch += DeltaTime;
	}

	//Lerp to origin.
	if (TimeSinceLaunch < LerpTime && LerpTime != 0)
	{
		MeshComponent->SetRelativeLocation(FMath::Lerp(InternalClientOffset, FVector::Zero(), TimeSinceLaunch / LerpTime));
	}
}
