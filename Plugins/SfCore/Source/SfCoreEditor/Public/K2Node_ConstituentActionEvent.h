// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_Event.h"
#include "UObject/Object.h"
#include "K2Node_ConstituentActionEvent.generated.h"

UCLASS()
class SFCOREEDITOR_API UK2Node_ConstituentActionEvent : public UK2Node_Event
{
	GENERATED_BODY()

public:

	/** Delegate property name that this event is associated with */
	UPROPERTY()
	FName DelegatePropertyName;

	/** Delegate property's owner class that this event is associated with */
	UPROPERTY()
	TObjectPtr<UClass> DelegateOwnerClass;

	/** Suffix given by ConstituentActionDelegateBinding to avoid event nodes with identical names. */
	UPROPERTY()
	FName UniqueEventSuffix;

	//~ Begin UObject Interface
	virtual bool Modify(bool bAlwaysMarkDirty = true) override;
	virtual void Serialize(FArchive& Ar) override;
	//~ End UObject Interface

	//~ Begin UEdGraphNode Interface
	virtual void ReconstructNode() override;
	virtual bool CanPasteHere(const UEdGraph* TargetGraph) const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual bool HasDeprecatedReference() const override;
	//~ End UEdGraphNode Interface

	//~ Begin K2Node Interface
	virtual bool NodeCausesStructuralBlueprintChange() const override { return true; }
	virtual UClass* GetDynamicBindingClass() const override;
	virtual void RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const override;
	//~ End K2Node Interface

	virtual bool IsUsedByAuthorityOnlyDelegate() const override;

	/** Return the delegate property that this event is bound to */
	FMulticastDelegateProperty* GetTargetDelegateProperty() const;

	/** Gets the proper display name for the property */
	FText GetTargetDelegateDisplayName() const;

	void InitializeConstituentActionEventParams(const FMulticastDelegateProperty* InDelegateProperty);

private:

	/** Constructing FText strings can be costly, so we cache the node's title */
	FNodeTextCache CachedNodeTitle;
};
