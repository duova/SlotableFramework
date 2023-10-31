// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SfCoreClasses.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "SfObject.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSfCore, Log, All);

/**
 * Base object for objects in the slotable hierarchy.
 * Routes remote function calls through the outer's NetDriver,
 * enables blueprint push model replication,
 * and provides utility functions.
 */
UCLASS()
class SFCORE_API USfObject : public UObject
{
	GENERATED_BODY()

public:

	USfObject();
	
	UFUNCTION(BlueprintPure)
	AActor* GetOwner() const
	{
		return GetTypedOuter<AActor>();
	}

	UFUNCTION(BlueprintPure)
	virtual UWorld* GetWorld() const override
	{
		return GetOwner() ? GetOwner()->GetWorld() : nullptr;
	}

	UFUNCTION(BlueprintPure)
	bool HasAuthority() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool IsSupportedForNetworking() const override;

	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;

	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Destroy();

	inline static constexpr int32 Int32MaxValue = 2147483647;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	AActor* SpawnActorInOwnerWorld(const TSubclassOf<AActor>& InClass, const FVector& Location, const FRotator& Rotation);
};