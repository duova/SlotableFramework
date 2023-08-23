// Fill out your copyright notice in the Description page of Project Settings.


#include "K2Node_ConstituentActionEvent.h"
// Copyright Epic Games, Inc. All Rights Reserved.

#include "ConstituentActionDelegateBinding.h"
#include "K2Node_ComponentBoundEvent.h"

#include "Containers/Array.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/DynamicBlueprintBinding.h"
#include "Engine/MemberReference.h"
#include "EngineLogs.h"
#include "SfObject.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformCrt.h"
#include "HAL/PlatformMath.h"
#include "Internationalization/Internationalization.h"
#include "Logging/LogCategory.h"
#include "Logging/LogMacros.h"
#include "Logging/MessageLog.h"
#include "Misc/AssertionMacros.h"
#include "Serialization/Archive.h"
#include "Templates/Casts.h"
#include "Templates/SubclassOf.h"
#include "Trace/Detail/Channel.h"
#include "UObject/Class.h"
#include "UObject/Field.h"
#include "UObject/Object.h"
#include "UObject/ObjectVersion.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "K2Node"

bool UK2Node_ConstituentActionEvent::Modify(bool bAlwaysMarkDirty)
{
	CachedNodeTitle.MarkDirty();

	return Super::Modify(bAlwaysMarkDirty);
}

bool UK2Node_ConstituentActionEvent::CanPasteHere(const UEdGraph* TargetGraph) const
{
	return false;
}
FText UK2Node_ConstituentActionEvent::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (CachedNodeTitle.IsOutOfDate(this))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("DelegatePropertyName"), GetTargetDelegateDisplayName());

		// FText::Format() is slow, so we cache this to save on performance
		CachedNodeTitle.SetCachedText(FText::Format(LOCTEXT("ConstituentActionEvent_Title", "{DelegatePropertyName}"), Args), this);
	}
	return CachedNodeTitle;
}

void UK2Node_ConstituentActionEvent::InitializeConstituentActionEventParams(const FMulticastDelegateProperty* InDelegateProperty)
{
	if (InDelegateProperty)
	{
		DelegatePropertyName = InDelegateProperty->GetFName();
		DelegateOwnerClass = CastChecked<UClass>(InDelegateProperty->GetOwner<UObject>())->GetAuthoritativeClass();
		
		EventReference.SetFromField<UFunction>(InDelegateProperty->SignatureFunction, true);

		CustomFunctionName = FName(*FString::Printf(TEXT("BndEvt__%s_%s_%s_%s"), *GetBlueprint()->GetName(), *GetName(), *EventReference.GetMemberName().ToString(), *UniqueEventSuffix.ToString()));
		bOverrideFunction = false;
		bInternalEvent = true;
		CachedNodeTitle.MarkDirty();
	}
}

UClass* UK2Node_ConstituentActionEvent::GetDynamicBindingClass() const
{
	return UConstituentActionDelegateBinding::StaticClass();
}

void UK2Node_ConstituentActionEvent::RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const
{
	UConstituentActionDelegateBinding* ConstituentActionBindingObject = CastChecked<UConstituentActionDelegateBinding>(BindingObject);

	FBlueprintConstituentActionDelegateBinding Binding;
	Binding.DelegatePropertyName = DelegatePropertyName;
	Binding.FunctionNameToBind = CustomFunctionName;

	CachedNodeTitle.MarkDirty();
	ConstituentActionBindingObject->ConstituentActionDelegateBindings.Add(Binding);
}

bool UK2Node_ConstituentActionEvent::HasDeprecatedReference() const
{
	return false;
}

bool UK2Node_ConstituentActionEvent::IsUsedByAuthorityOnlyDelegate() const
{
	return false;
}

FMulticastDelegateProperty* UK2Node_ConstituentActionEvent::GetTargetDelegateProperty() const
{
	return FindFProperty<FMulticastDelegateProperty>(DelegateOwnerClass, DelegatePropertyName);
}

FText UK2Node_ConstituentActionEvent::GetTargetDelegateDisplayName() const
{
	FMulticastDelegateProperty* Prop = GetTargetDelegateProperty();
	return Prop ? Prop->GetDisplayNameText() : FText::FromName(DelegatePropertyName);
}

FText UK2Node_ConstituentActionEvent::GetTooltipText() const
{
	FMulticastDelegateProperty* TargetDelegateProp = GetTargetDelegateProperty();
	if (TargetDelegateProp)
	{
		return TargetDelegateProp->GetToolTipText();
	}
	else
	{
		return FText::FromName(DelegatePropertyName);
	}
}

void UK2Node_ConstituentActionEvent::ReconstructNode()
{
	// We need to fixup our event reference as it may have changed or been redirected
	FMulticastDelegateProperty* TargetDelegateProp = GetTargetDelegateProperty();

	// If we couldn't find the target delegate, then try to find it in the property remap table
	if (!TargetDelegateProp)
	{
		FMulticastDelegateProperty* NewProperty = FMemberReference::FindRemappedField<FMulticastDelegateProperty>(DelegateOwnerClass, DelegatePropertyName);
		if (NewProperty)
		{
			// Found a remapped property, update the node
			TargetDelegateProp = NewProperty;
			DelegatePropertyName = NewProperty->GetFName();
		}
	}

	if (TargetDelegateProp && TargetDelegateProp->SignatureFunction)
	{
		EventReference.SetFromField<UFunction>(TargetDelegateProp->SignatureFunction, true);
	}

	CachedNodeTitle.MarkDirty();

	Super::ReconstructNode();
}

void UK2Node_ConstituentActionEvent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
}


#undef LOCTEXT_NAMESPACE
