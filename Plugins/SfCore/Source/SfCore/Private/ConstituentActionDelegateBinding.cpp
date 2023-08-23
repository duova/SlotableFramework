// Fill out your copyright notice in the Description page of Project Settings.


#include "ConstituentActionDelegateBinding.h"

UConstituentActionDelegateBinding::UConstituentActionDelegateBinding(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UConstituentActionDelegateBinding::BindDynamicDelegates(UObject* InInstance) const
{
	for (int32 BindIdx = 0; BindIdx < ConstituentActionDelegateBindings.Num(); BindIdx++)
	{
		const FBlueprintConstituentActionDelegateBinding& Binding = ConstituentActionDelegateBindings[BindIdx];

		// Get delegate property on constituent
		if (FMulticastDelegateProperty* MulticastDelegateProp = FindFProperty<FMulticastDelegateProperty>(
			InInstance->GetClass(), Binding.DelegatePropertyName))
		{
			// Get the function we want to bind
			if (UFunction* FunctionToBind = InInstance->GetClass()->FindFunctionByName(Binding.FunctionNameToBind))
			{
				// Bind function on the instance to this delegate
				FScriptDelegate Delegate;
				Delegate.BindUFunction(InInstance, Binding.FunctionNameToBind);
				MulticastDelegateProp->AddDelegate(MoveTemp(Delegate), InInstance);
			}
		}
	}
}

void UConstituentActionDelegateBinding::UnbindDynamicDelegates(UObject* InInstance) const
{
	for (int32 BindIdx = 0; BindIdx < ConstituentActionDelegateBindings.Num(); BindIdx++)
	{
		const FBlueprintConstituentActionDelegateBinding& Binding = ConstituentActionDelegateBindings[BindIdx];

		// Get delegate property on constituent
		if (FMulticastDelegateProperty* MulticastDelegateProp = FindFProperty<FMulticastDelegateProperty>(
			InInstance->GetClass(), Binding.DelegatePropertyName))
		{
			// Unbind function on the instance to this delegate
			FScriptDelegate Delegate;
			Delegate.BindUFunction(InInstance, Binding.FunctionNameToBind);
			MulticastDelegateProp->RemoveDelegate(Delegate, InInstance);
		}
	}
}

void UConstituentActionDelegateBinding::UnbindDynamicDelegatesForProperty(
	UObject* InInstance, const FObjectProperty* InObjectProperty) const
{
	for (int32 BindIdx = 0; BindIdx < ConstituentActionDelegateBindings.Num(); BindIdx++)
	{
		const FBlueprintConstituentActionDelegateBinding& Binding = ConstituentActionDelegateBindings[BindIdx];
		if (InObjectProperty->GetFName() == Binding.DelegatePropertyName)
		{
			// Get delegate property on constituent
			if (FMulticastDelegateProperty* MulticastDelegateProp = FindFProperty<FMulticastDelegateProperty>(
				InInstance->GetClass(), Binding.DelegatePropertyName))
			{
				// Unbind function on the instance from this delegate
				FScriptDelegate Delegate;
				Delegate.BindUFunction(InInstance, Binding.FunctionNameToBind);
				MulticastDelegateProp->RemoveDelegate(Delegate, InInstance);
			}
			break;
		}
	}
}
