// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SfObject.generated.h"

/**
 * Base object for objects in the slotable hierarchy.
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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool IsSupportedForNetworking() const override;

	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;

	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Destroy();

protected:
	void ErrorIfNoAuthority() const;
};
