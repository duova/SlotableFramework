// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DynamicBlueprintBinding.h"
#include "UObject/Object.h"
#include "ConstituentActionDelegateBinding.generated.h"

/** Entry for a delegate to assign after a blueprint has been instanced */
USTRUCT()
struct SFCORE_API FBlueprintConstituentActionDelegateBinding
{
	GENERATED_USTRUCT_BODY()

	/** Name of property on the component that we want to assign to. */
	UPROPERTY()
	FName DelegatePropertyName;

	/** Name of function that we want to bind to the delegate. */
	UPROPERTY()
	FName FunctionNameToBind;

	FBlueprintConstituentActionDelegateBinding()
		: DelegatePropertyName(NAME_None)
		, FunctionNameToBind(NAME_None)
	{ }
};

UCLASS()
class SFCORE_API UConstituentActionDelegateBinding : public UDynamicBlueprintBinding
{
	GENERATED_UCLASS_BODY()
	
	UPROPERTY()
	TArray<FBlueprintConstituentActionDelegateBinding> ConstituentActionDelegateBindings;

	//~ Begin DynamicBlueprintBinding Interface
	virtual void BindDynamicDelegates(UObject* InInstance) const override;
	virtual void UnbindDynamicDelegates(UObject* InInstance) const override;
	virtual void UnbindDynamicDelegatesForProperty(UObject* InInstance, const FObjectProperty* InObjectProperty) const override;
	//~ End DynamicBlueprintBinding Interface
};