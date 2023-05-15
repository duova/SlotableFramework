// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SfCoreClasses.h"
#include "SfObject.generated.h"

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
	UFUNCTION(BlueprintPure)
	AActor* GetOwner() const
	{
		return GetTypedOuter<AActor>();
	}

	UFUNCTION(BlueprintPure)
	bool HasAuthority() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool IsSupportedForNetworking() const override;

	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;

	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Destroy();

	class UFormCharacterComponent* GetFormCharacter() const;
	
	bool IsFormCharacter() const;

protected:
	void ErrorIfNoAuthority() const;
};
