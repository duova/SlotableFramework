// Fill out your copyright notice in the Description page of Project Settings.


#include "K2Node_ConstituentActionEvent.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "Constituent.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphSchema_K2_Actions.h"
#include "GraphEditorSettings.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Self.h"
#include "KismetCompiler.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"

UK2Node_ConstituentActionEvent::UK2Node_ConstituentActionEvent()
{
	ActionId = 1;
	ActionExecutionContext = EActionExecutionContext::Server;
	ActionExecutionPerspective = EActionExecutionPerspective::All;
}

FString UK2Node_ConstituentActionEvent::ActionExecutionContextAsString(const EActionExecutionContext ExecutionContext)
{
	switch (ExecutionContext)
	{
	case EActionExecutionContext::Server:
		return FString("Server");
	case EActionExecutionContext::Predicted:
		return FString("Predicted");
	case EActionExecutionContext::Autonomous:
		return FString("Autonomous");
	case EActionExecutionContext::Simulated:
		return FString("Simulated");
	default:
		return FString("");
	}
}

FString UK2Node_ConstituentActionEvent::ActionExecutionPerspectiveAsString(
	const EActionExecutionPerspective ExecutionPerspective)
{
	switch (ExecutionPerspective)
	{
	case EActionExecutionPerspective::All:
		return FString("All");
	case EActionExecutionPerspective::FirstPerson:
		return FString("FirstPerson");
	case EActionExecutionPerspective::ThirdPerson:
		return FString("ThirdPerson");
	default:
		return FString("");
	}
}

void UK2Node_ConstituentActionEvent::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, TEXT("Execute"));
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, TEXT("bIsReplaying"));
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Real, TEXT("TimeSinceExecution"));
}

FLinearColor UK2Node_ConstituentActionEvent::GetNodeTitleColor() const
{
	return GetDefault<UGraphEditorSettings>()->EventNodeTitleColor;
}

FText UK2Node_ConstituentActionEvent::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::MenuTitle)
	{
		return FText::FromName(NodeMenuName);
	}
	if (CachedNodeTitle.IsOutOfDate(this))
	{
		const FText Title = FText::Format(
			         NSLOCTEXT("K2Node", "ConstituentActionEvent_Name", "Action {0} On {1} {2} Perspective(s)"),
			         FText::AsNumber(ActionId), FText::FromString(ActionExecutionContextAsString(ActionExecutionContext)), FText::FromString(ActionExecutionPerspectiveAsString(ActionExecutionPerspective)));
		CachedNodeTitle.SetCachedText(Title, this);
	}

	return CachedNodeTitle;
}

FText UK2Node_ConstituentActionEvent::GetTooltipText() const
{
	if (CachedTooltip.IsOutOfDate(this))
	{
		CachedTooltip.SetCachedText(NSLOCTEXT("K2Node", "ConstituentActionEvent_Tooltip", "Event for when the action specified is executed with the certain context and perspective."), this);
	}
	return CachedTooltip;
}

FSlateIcon UK2Node_ConstituentActionEvent::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon(FAppStyle::GetAppStyleSetName(), "GraphEditor.Event_16x");
	return Icon;
}

bool UK2Node_ConstituentActionEvent::IsCompatibleWithGraph(UEdGraph const* Graph) const
{
	EGraphType const GraphType = Graph->GetSchema()->GetGraphType(Graph);
	bool bIsCompatible = (GraphType == EGraphType::GT_Ubergraph);

	if (bIsCompatible)
	{
		UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);

		UEdGraphSchema_K2 const* K2Schema = Cast<UEdGraphSchema_K2>(Graph->GetSchema());
		bool const bIsConstructionScript = (K2Schema != nullptr) ? UEdGraphSchema_K2::IsConstructionScript(Graph) : false;

		//Check that the ubergraph belongs to a class that extends UConstituent.
		bIsCompatible = (Blueprint != nullptr) && Blueprint->ParentClass.Get() && Blueprint->ParentClass.Get()->IsChildOf(UConstituent::StaticClass()) && !bIsConstructionScript && Super::IsCompatibleWithGraph(Graph);
	}
	return bIsCompatible;
}

UEdGraphPin* UK2Node_ConstituentActionEvent::GetExecutePin() const
{
	return FindPin(TEXT("Execute"));
}

UEdGraphPin* UK2Node_ConstituentActionEvent::GetReplayPin() const
{
	return FindPin(TEXT("bIsReplaying"));
}

UEdGraphPin* UK2Node_ConstituentActionEvent::GetTimePin() const
{
	return FindPin(TEXT("TimeSinceExecution"));
}

void UK2Node_ConstituentActionEvent::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	
}

void UK2Node_ConstituentActionEvent::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	//Build expanded node graph.
	
	//Create event to execute the expanded node.
	UK2Node_CustomEvent* ExecuteEventNode = CompilerContext.SpawnIntermediateEventNode<UK2Node_CustomEvent>(this, GetExecutePin(), SourceGraph);
	
	//Set the event to the corresponding event on UConstituent.
	switch (ActionExecutionContext)
	{
	case EActionExecutionContext::Server:
		ExecuteEventNode->SetDelegateSignature(UConstituent::StaticClass()->FindFunctionByName(ServerExecuteFunctionName));
		break;
	case EActionExecutionContext::Predicted:
		ExecuteEventNode->SetDelegateSignature(UConstituent::StaticClass()->FindFunctionByName(PredictedExecuteFunctionName));
		break;
	case EActionExecutionContext::Autonomous:
		ExecuteEventNode->SetDelegateSignature(UConstituent::StaticClass()->FindFunctionByName(AutonomousExecuteFunctionName));
		break;
	case EActionExecutionContext::Simulated:
		ExecuteEventNode->SetDelegateSignature(UConstituent::StaticClass()->FindFunctionByName(SimulatedExecuteFunctionName));
		break;
	}
	ExecuteEventNode->AllocateDefaultPins();
	
	//Create branch node to only allow execution if the conditions match.
	UK2Node_IfThenElse* BranchNode = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);
	BranchNode->AllocateDefaultPins();
	
	//Connect the exec pin of the branch node.
	UEdGraphPin* EventNodeExecPin = ExecuteEventNode->GetThenPin();
	UEdGraphPin* BranchInExec = BranchNode->GetExecPin();
	checkf(BranchInExec, TEXT("BranchInExec is null."));
	K2Schema->TryCreateConnection(EventNodeExecPin, BranchInExec);
	
	//Create equals node to check if action id matches.
	UK2Node_CallFunction* IntEqualEqualNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	IntEqualEqualNode->FunctionReference.SetExternalMember(EqualEqualIntIntFunctionName, UKismetMathLibrary::StaticClass());
	IntEqualEqualNode->AllocateDefaultPins();

	//Connect action id pin to first pin on int equals.
	UEdGraphPin* ActionIdPin = ExecuteEventNode->FindPinChecked(ActionIdPinName, EGPD_Output);
	UEdGraphPin* FirstEqualsInputPin = IntEqualEqualNode->FindPinChecked(FirstEqualsInputPinName, EGPD_Input);
	K2Schema->TryCreateConnection(ActionIdPin, FirstEqualsInputPin);

	//Set second equals value to the given action id.
	UEdGraphPin* SecondEqualsInputPin = IntEqualEqualNode->FindPinChecked(SecondEqualsInputPinName, EGPD_Input);
	SecondEqualsInputPin->DefaultValue = FString::FromInt(ActionId);

	UEdGraphPin* BranchConditionPin = BranchNode->GetConditionPin();

	UEdGraphPin* EqualsOutPin = IntEqualEqualNode->GetReturnValuePin();
	
	//For Server or if we would want to execute for all perspectives, we directly connect the output of the equals node to the condition pin on the branch node.
	if (ActionExecutionContext == EActionExecutionContext::Server || ActionExecutionPerspective == EActionExecutionPerspective::All)
	{
		K2Schema->TryCreateConnection(EqualsOutPin, BranchConditionPin);
	}
	else
	{
		//Otherwise we create a bool AND to combine the action id condition and perspective conditions.
		UK2Node_CallFunction* BoolAndNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		BoolAndNode->FunctionReference.SetExternalMember(BoolAndFunctionName, UKismetMathLibrary::StaticClass());
		BoolAndNode->AllocateDefaultPins();

		//Connect the return of bool AND to the branch condition pin.
		UEdGraphPin* BoolAndOutPin = BoolAndNode->GetReturnValuePin();
		K2Schema->TryCreateConnection(BoolAndOutPin, BranchConditionPin);

		//Connect the first input of bool AND to the output of int equals.
		UEdGraphPin* BoolAndFirstInPin = BoolAndNode->FindPinChecked(FirstBoolAndInputPinName, EGPD_Input);
		K2Schema->TryCreateConnection(EqualsOutPin, BoolAndFirstInPin);

		//We set this pointer based on which pin provides the correct boolean for the perspective.
		UEdGraphPin* PerspectiveConditionMetPin;
		UEdGraphPin* EventIsFirstPersonPin = ExecuteEventNode->FindPinChecked(IsFirstPersonPinName, EGPD_Output);
		if (ActionExecutionPerspective == EActionExecutionPerspective::FirstPerson)
		{
			//We can just use the pin from the event directly.
			PerspectiveConditionMetPin = EventIsFirstPersonPin;
		}
		else
		{
			//We create a NOT bool node and set the pin to the output.
			UK2Node_CallFunction* NotBoolNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
			NotBoolNode->FunctionReference.SetExternalMember(NotBoolFunctionName, UKismetMathLibrary::StaticClass());
			NotBoolNode->AllocateDefaultPins();
			PerspectiveConditionMetPin = NotBoolNode->GetReturnValuePin();
			
			//Connect NotBoolNode input to the event perspective pin.
			UEdGraphPin* NotBoolInputPin = NotBoolNode->FindPinChecked(NotBoolInputPinName, EGPD_Input);
			K2Schema->TryCreateConnection(NotBoolInputPin, EventIsFirstPersonPin);
		}

		UEdGraphPin* BoolAndSecondInPin = BoolAndNode->FindPinChecked(SecondBoolAndInputPinName, EGPD_Input);
		if (ActionExecutionContext != EActionExecutionContext::Predicted)
		{
			//If not predicted we can simply connect the PerspectiveConditionMetPin to the BoolAndNode.
			K2Schema->TryCreateConnection(BoolAndSecondInPin, PerspectiveConditionMetPin);
		}
		else
		{
			//If predicted we need to ensure that on the server it gets called no matter the perspective.
			//An OR node has to be added between bIsServer, PerspectiveConditionMetPin, and the BoolAndNode.
			UK2Node_CallFunction* OrBoolNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
			OrBoolNode->FunctionReference.SetExternalMember(BoolOrFunctionName, UKismetMathLibrary::StaticClass());
			OrBoolNode->AllocateDefaultPins();

			UEdGraphPin* OrBoolFirstInPin = OrBoolNode->FindPinChecked(FirstBoolOrInputPinName, EGPD_Input);
			UEdGraphPin* OrBoolSecondInPin = OrBoolNode->FindPinChecked(SecondBoolOrInputPinName, EGPD_Input);
			UEdGraphPin* OrBoolOutPin = OrBoolNode->GetReturnValuePin();
			UEdGraphPin* ExecuteEventIsServerPin = ExecuteEventNode->FindPinChecked(IsServerPinName, EGPD_Output);

			K2Schema->TryCreateConnection(OrBoolFirstInPin, PerspectiveConditionMetPin);
			K2Schema->TryCreateConnection(OrBoolSecondInPin, ExecuteEventIsServerPin);
			K2Schema->TryCreateConnection(OrBoolOutPin, BoolAndSecondInPin);
		}
	}

	for (auto Element : ExecuteEventNode->Pins)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Element->PinName.ToString());
	}

	//Move pins to intermediates.
	CompilerContext.MovePinLinksToIntermediate(*GetExecutePin(), *BranchNode->FindPinChecked(UEdGraphSchema_K2::PN_Then));
	if (ActionExecutionContext == EActionExecutionContext::Predicted)
	{
		CompilerContext.MovePinLinksToIntermediate(*GetReplayPin(), *ExecuteEventNode->FindPinChecked(IsReplayingPinName, EGPD_Output));
	}
	else if (ActionExecutionContext != EActionExecutionContext::Server)
	{
		CompilerContext.MovePinLinksToIntermediate(*GetTimePin(), *ExecuteEventNode->FindPinChecked(TimeSincePinName, EGPD_Output));
	}

	this->BreakAllNodeLinks();
}

void UK2Node_ConstituentActionEvent::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	const UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FText UK2Node_ConstituentActionEvent::GetMenuCategory() const
{
	static FNodeTextCache CachedCategory;
	if (CachedCategory.IsOutOfDate(this))
	{
		CachedCategory.SetCachedText(NSLOCTEXT("K2Node", "ConstituentActionEvent_MenuCatagory", "Constituent Actions"), this);
	}
	return CachedCategory;
}

FBlueprintNodeSignature UK2Node_ConstituentActionEvent::GetSignature() const
{
	FBlueprintNodeSignature NodeSignature = Super::GetSignature();
	NodeSignature.AddKeyValue(NodeMenuName.ToString());

	return NodeSignature;
}

TSharedPtr<FEdGraphSchemaAction> UK2Node_ConstituentActionEvent::GetEventNodeAction(const FText& ActionCategory)
{
	TSharedPtr<FEdGraphSchemaAction_K2InputAction> EventNodeAction = MakeShareable(new FEdGraphSchemaAction_K2InputAction(ActionCategory, GetNodeTitle(ENodeTitleType::EditableTitle), GetTooltipText(), 0));
	EventNodeAction->NodeTemplate = this;
	return EventNodeAction;
}

