// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Constituent.h"
#include "GameplayTagContainer.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "SfProjectile.generated.h"

USTRUCT(BlueprintType)
struct FSfProjectileParams
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<AActor*> ActorsToIgnore;

	TArray<FGameplayTag> TeamsToIgnore;

	//Passes through but still calls event.
	UPROPERTY()
	TArray<AActor*> ActorsToPassthrough;

	//Passes through but still calls event.
	TArray<FGameplayTag> TeamsToPassthrough;
};

DECLARE_DYNAMIC_DELEGATE_SixParams(FProjectileOverlap, AActor*, Actor, UPrimitiveComponent*, PrimitiveComponent, ASfProjectile*, Projectile, FVector, ProjectileLocation, FVector, ProjectileVelocity, bool, bIsPassthrough);

/*
 * Projectile spawned with Server_SpawnSfProjectile.
 * Mainly allows for different starting points based on the perspective which gets interpolated to the authoritative projectile.
 * Requires a UMeshComponent to function.
 */
UCLASS(Blueprintable, Abstract)
class SFTARGETING_API ASfProjectile : public AActor
{
	GENERATED_BODY()

public:
	ASfProjectile();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//Spawns a implemented SfProjectile for a UConstituent.
	//InOrigin is where the projectile spawns on the server, though the client versions depend on
	//the Fp(first person) or Tp(third person) local space offsets on the projectile.
	UFUNCTION(BlueprintCallable, meta = (DefaultToSelf = "Target"))
	static ASfProjectile* Server_SpawnSfProjectile(UConstituent* Target, const TSubclassOf<ASfProjectile>& InClass,
	                                                  const FVector& InOrigin, const FVector& InDirection,
	                                                  const float InVelocity, const FSfProjectileParams& InParams, const FProjectileOverlap OnOverlap);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void Overlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_ConstituentOwner();

	UPROPERTY(ReplicatedUsing = OnRep_ConstituentOwner)
	UConstituent* ConstituentOwner;

	UPROPERTY()
	UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY()
	USphereComponent* CollisionComponent;

	UPROPERTY()
	UMeshComponent* MeshComponent;

	float TimeSinceLaunch = 0;

	FProjectileOverlap OverlapEvent;

	FOnEnterOverlapSignature InternalOverlap;

	FSfProjectileParams Params;

	//Based on the local space of the projectile.
	UPROPERTY(EditAnywhere)
	FVector TpOffset;

	//Based on the local space of the projectile.
	UPROPERTY(EditAnywhere)
	FVector FpOffset;

	FVector InternalClientOffset;

	//Time for the client projectile location to rejoin the server location.
	UPROPERTY(EditAnywhere)
	float LerpTime;
	
public:
	virtual void Tick(float DeltaTime) override;
};
