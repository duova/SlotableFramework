// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfObject.h"
#include "Constituent.generated.h"

class USfQuery;
class UFormCoreComponent;
class UFormCharacterComponent;

//Set of maximum four action identifiers that are compressed on serialization if possible.
USTRUCT()
struct SFCORE_API FActionSet
{
	GENERATED_BODY()

	uint8 NumActionsIncludingZero; //Serialized to 2 bits.

	uint8 ActionZero; //Serialized to 6 bits.

	uint8 ActionOne; //Serialized to 6 bits.

	uint8 ActionTwo; //Serialized to 6 bits.

	uint8 ActionThree; //Serialized to 6 bits.

	//Used for checking whether actions were performed in the same frame.
	float WorldTime;

	//Since replication needs a variable to change in order to replicate and OnRep, we flip this bool when we want to
	//force send changes.
	uint8 bFlipToForceReplicate:1;

	//Do not use default constructor.
	FActionSet();

	FActionSet(float CurrentWorldTime, uint8 ActionZero, uint8 ActionOne = 0, uint8 ActionTwo = 0, uint8 ActionThree = 0);

	//Returns false if the action was created on another frame.
	bool TryAddActionCheckIfSameFrame(float CurrentWorldTime, uint8 Action);

	bool operator==(const FActionSet& Other) const;

	TArray<uint8> ToArray() const;

	friend FArchive& operator<<(FArchive& Ar, FActionSet& Set)
	{
		Ar.SerializeBits(&Set.NumActionsIncludingZero, 2);
		Ar.SerializeBits(&Set.ActionZero, 6);
		if (Set.NumActionsIncludingZero > 0)
		{
			Ar.SerializeBits(&Set.ActionOne, 6);
		}
		if (Set.NumActionsIncludingZero > 1)
		{
			Ar.SerializeBits(&Set.ActionTwo, 6);
		}
		if (Set.NumActionsIncludingZero > 2)
		{
			Ar.SerializeBits(&Set.ActionThree, 6);
		}
		return Ar;
	}
};

/**
 * Building blocks of a slotable which can be reused to share functionality between slotables.
 * These are supposed to be scriptable in a blueprint class with targeters, operators, and events.
 *
 * ***Details***
 *
 * The behaviour of a constituent is defined with actions. Conceptually every action represents a discrete cause and
 * effect that happens with the constituent. Examples of this are a spell being casted, a gun being fired, a reload
 * starting, a reload finishing, and cooldown ending. Most things that happen can be considered an action, however
 * simultaneous actions should be considered a singular action. For example, a spell being casted and the cooldown
 * starting are represented by executing the same action as they happen simultaneously. Implementing a constituent in
 * blueprints involves defining the condition of each actions, then implementing the results of execution.
 * Note: Events that directly change properties/variables of a constituent on the server can just be that. An action is
 * generally excessive for a direct server blueprint variable change.
 *
 * ***Condition Graph***
 * 
 * The condition graph of an action determines when the execution should happen. Blueprint events are provided as inputs
 * to a condition graph, which are processed by nodes that check different conditions as desired by the designer, and finally
 * calls the ExecuteAction blueprint function once conditions are reached. As a rule, condition graphs may not make any
 * changes to the game state directly, including visual changes. All actions are identified by a positive integer between
 * 1-63, and this is what links condition graphs with execution graphs. Action identifiers cannot be repeated, but they
 * do not have to be ordered. If 0 is used, it will instantly trigger with potentially unexpected consequences.
 *
 * ***Guidelines On Simultaneous Actions***
 *
 * Actions that are expected to happen simultaneously should be combined. The number of actions that have the possibility
 * of happening simultaneously should be reduced to less than four with the following options:
 * - Input related actions can typically be combined.
 * - If it is simply a server variable change linked to a non-input event, it does not have to be an action. Simply connect
 * the variable change to the event.
 * - Use another constituent. At this point there are likely too many actions in one constituent anyway. Constituents can
 * reference other constituents through their common slotable.
 *
 * ***Execution Graph***
 *
 * Note: FP is first person and TP is third person
 *
 * The results of executing an action is defined by an execution graph. This begins with a blueprint event that indicates
 * which action has been executed and which network role this execution graph is a definition for. The server network
 * role can use any node in order to produce any effects on the server, which should be replicated to the client if relevant.
 * Note that all replication in the SF is done with the push model. The simulated FP, simulated TP, and autonomous roles are
 * limited to nodes that are for presentation and not simulation. Four execution graphs, one for each role, should be implemented
 * for each state that the constituent can enter. In addition, the predicted role, which executes on both the server and
 * autonomous client conditionally, can also be implemented only with nodes marked Predicted_. Neither the autonomous role
 * or the predicted role have to be implemented for non-player forms. Game state condition checking should always be
 * done completely by the condition graph before execution, so execution graphs should not have game state condition checks,
 * only presentation related checks. (eg. a stat check should be in the condition graph, but a check of whether a UI section
 * is overflowing should be in the execution graph) In addition, execution should be instantaneous, and delayed effects
 * should be put into another state (many SF nodes have a delay variable that can be used for short delays). Audiovisual
 * effects obviously cannot be instantaneous, but they should be called instantaneously.
 *
 * Note: SF has special implementations for audiovisual effects (including audio, niagara, animations, and material
 * property changes). These should be used instead of directly producing these effects. The nodes are marked Predicted_,
 * Simulated_, and Autonomous_ (the last one is more rare since usually they are Predicted). They are designed to guarantee
 * that the effects work with complex networking scenarios such as rollback and relevancy.
 */
UCLASS(Blueprintable)
class SFCORE_API UConstituent : public USfObject
{
	GENERATED_BODY()

public:

	UConstituent();
	
	virtual void BeginDestroy() override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_OwningSlotable, Replicated, BlueprintReadOnly, VisibleAnywhere)
	class USlotable* OwningSlotable;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	uint8 bEnableInputsAndPrediction:1;

	//Note: Slotables should async load all assets on init and unload on deinit.
	void ClientInitialize();

	void ServerInitialize();
	
	void ClientDeinitialize();

	void ServerDeinitialize();
	
	UFUNCTION(BlueprintCallable)
	void ExecuteAction(const uint8 ActionId, const bool bIsPredictableContext);

	UFUNCTION(BlueprintImplementableEvent)
	void Server_OnExecute(const uint8 ActionId);

	//Only use functions marked Predicted_.
	UFUNCTION(BlueprintImplementableEvent)
	void Predicted_OnExecute(const uint8 ActionId, const bool bIsReplaying);

	//TimeSinceLastActionSet does not increment past 655 seconds.
	
	UFUNCTION(BlueprintImplementableEvent)
	void Autonomous_OnExecute(const uint8 ActionId, const float TimeSinceExecution);

	UFUNCTION(BlueprintImplementableEvent)
	void SimulatedFP_OnExecute(const uint8 ActionId, const float TimeSinceExecution);

	UFUNCTION(BlueprintImplementableEvent)
	void SimulatedTP_OnExecute(const uint8 ActionId, const float TimeSinceExecution);

	UFUNCTION(BlueprintImplementableEvent)
	void OnInputDown(const bool bIsPredictableContext);

	UFUNCTION(BlueprintImplementableEvent)
	void OnInputUp(const bool bIsPredictableContext);

	static void ErrorIfIdNotWithinRange(const uint8 Id);

	UFUNCTION(BlueprintGetter)
	uint8 GetInstanceId() const;
	
	UFUNCTION()
	void OnRep_LastActionSet();

	void IncrementTimeSincePredictedLastActionSet(float Time);

	//Unique identifier within each inventory.
	UPROPERTY(Replicated)
	uint8 InstanceId;

	//We use ActionSet instead of just uint8 for multiple reasons. Most importantly we need to do so as more than one action
	//can be executed within a frame. ActionSet allows us to work with up to four actions that execute in the same frame,
	//which should guarantee actions should not be skipped assuming the guidelines for creating them are followed. ActionSet
	//performs serialization with lossless compression and checks that only actions from the same frame are added.
	
	//This is changed during a predicted execution. Client version is sent to the server via FormCharacter, checked, added
	//to LastActionSet of that frame, and corrected to the client if necessary.
	FActionSet PredictedLastActionSet;

	//Set to true when we predict an action. Set to false when FormCharacter collects pending actions.
	uint8 bPredictedLastActionSetUpdated:1;

	//Used to know how long ago the last action took place so we can fast forward the effects after replay.
	FUint16_Quantize100 TimeSincePredictedLastActionSet;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<USfQuery>> QueryDependencies;
	
private:
	UFUNCTION()
	void OnRep_OwningSlotable();

	uint8 bAwaitingClientInit:1;

	//Actions performed on the last frame any actions were performed.
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_LastActionSet)
	FActionSet LastActionSet;

	//Only mark dirty when LastActionSet is changed. This is for replicating effects that started right before relevancy.
	//Uses replicated non-compensated timestamp found in form core.
	UPROPERTY(Replicated)
	float LastActionSetTimestamp;

	UPROPERTY()
	UFormCoreComponent* FormCore;
	
	//In case the naming is confusing:
	//On the server, neither "time since" is relevant because actions are always executed instantly. On the simulated proxy,
	//we always use LastActionSet paired with TimeSinceLastActionSet because prediction isn't running. On the autonomous proxy,
	//TimeSinceLastActionSet will almost always be 0 since we're always relevant, so we only recreate the effects from
	//PredictedLastActionSet using the respective "time since" and let OnRep handle LastActionSet changes.
};
