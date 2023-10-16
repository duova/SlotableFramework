// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_Event.h"
#include "K2Node_ConstituentAction.generated.h"

UENUM(BlueprintType)
enum class EActionExecutionContext : uint8
{
	Server,
	Predicted,
	Autonomous,
	Simulated
};

UENUM(BlueprintType)
enum class EActionExecutionPerspective : uint8
{
	All,
	FirstPerson,
	ThirdPerson
};

/**
 * Event node that is called dynamically based on what UConstituent action and NetRole context it is subscribed to.
 */
UCLASS()
class SFCOREEDITOR_API UK2Node_ConstituentAction : public UK2Node, public IK2Node_EventNodeInterface
{
	GENERATED_BODY()
	
	UK2Node_ConstituentAction();

	UPROPERTY()
	FName NodeMenuName = FName("ConstituentActionEvent");

	//Action to subscribe to.
	UPROPERTY(EditAnywhere, Category="Constituent", meta = (ClampMin = 1, ClampMax = 63))
	int32 ActionId;

	//Context to subscribe to.
	UPROPERTY(EditAnywhere, Category="Constituent")
	EActionExecutionContext ActionExecutionContext;

	//Perspective(s) to run this on.
	UPROPERTY(EditAnywhere, Category="Constituent")
	EActionExecutionPerspective ActionExecutionPerspective;

	//Param struct type.
	UPROPERTY(EditAnywhere, Category="Constituent")
	TObjectPtr<UScriptStruct> ParamsStructType;
	
	inline static const FName ActionIdPinName = "InActionId";
	inline static const FName IsServerPinName = "bIsServer";
	inline static const FName IsReplayingPinName = "bInIsReplaying";
	inline static const FName TimeSincePinName = "InTimeSinceExecution";
	inline static const FName ParamsPinName = "Params";
	inline static const FName ParamsGetterPinName = "OutStruct";
	inline static const FName EqualEqualIntIntFunctionName = "EqualEqual_IntInt";
	inline static const FName FirstEqualsInputPinName = "A";
	inline static const FName SecondEqualsInputPinName = "B";
	inline static const FName BoolAndFunctionName = "BooleanAND";
	inline static const FName FirstBoolAndInputPinName = "A";
	inline static const FName SecondBoolAndInputPinName = "B";
	inline static const FName IsFirstPersonPinName = "bIsFirstPerson";
	inline static const FName NotBoolFunctionName = "Not_PreBool";
	inline static const FName NotBoolInputPinName = "A";
	inline static const FName BoolOrFunctionName = "BooleanOR";
	inline static const FName FirstBoolOrInputPinName = "A";
	inline static const FName SecondBoolOrInputPinName = "B";

	static FString ActionExecutionContextAsString(const EActionExecutionContext ExecutionContext);

	static FString ActionExecutionPerspectiveAsString(const EActionExecutionPerspective ExecutionPerspective);
	
	//~ Begin UEdGraphNode Interface.
	virtual void AllocateDefaultPins() override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	//~ End UEdGraphNode Interface.

	//~ Begin UK2Node Interface
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
	virtual bool ShouldShowNodeProperties() const override { return true; }
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	virtual FBlueprintNodeSignature GetSignature() const override;
	virtual bool IsNodeSafeToIgnore() const override { return true; }
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void ReconstructNode() override;
	//~ End UK2Node Interface

	//~ Begin IK2Node_EventNodeInterface Interface.
	virtual TSharedPtr<FEdGraphSchemaAction> GetEventNodeAction(const FText& ActionCategory) override;
	//~ End IK2Node_EventNodeInterface Interface.

	UEdGraphPin* GetExecutePin() const;
	UEdGraphPin* GetReplayPin() const;
	UEdGraphPin* GetTimePin() const;
	UEdGraphPin* GetParamsPin();

private:
	/** Constructing FText strings can be costly, so we cache the node's title/tooltip */
	FNodeTextCache CachedTooltip;
	FNodeTextCache CachedNodeTitle;
};
