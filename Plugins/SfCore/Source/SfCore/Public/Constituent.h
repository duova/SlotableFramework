// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfObject.h"
#include "SfUtility.h"
#include "Constituent.generated.h"

class UConstituent;
class UInventory;
class UCardObject;
class USfQuery;
class UFormCoreComponent;
class UFormCharacterComponent;

//Set of maximum four action identifiers that are compressed on serialization if possible.
USTRUCT()
struct SFCORE_API FActionSet
{
	GENERATED_BODY()

	uint8 NumActionsMinusOne; //Serialized to 2 bits.

	uint8 ActionZero; //Serialized to 6 bits.

	TBitArray<> ActionZeroParams;

	uint8 ActionOne; //Serialized to 6 bits.

	TBitArray<> ActionOneParams;

	uint8 ActionTwo; //Serialized to 6 bits.

	TBitArray<> ActionTwoParams;

	uint8 ActionThree; //Serialized to 6 bits.

	TBitArray<> ActionThreeParams;
	
	//Used for checking whether actions were performed in the same frame.
	float WorldTime;

	//Do not use default constructor.
	FActionSet();

	FActionSet(const float InCurrentWorldTime, const uint8 InActionZero, const uint8 InActionOne = 0,
	           const uint8 InActionTwo = 0,
	           const uint8 InActionThree = 0);

	FActionSet(const float InCurrentWorldTime, const uint8 InActionZero, const FBitWriter& InActionZeroParams = FBitWriter(),
	           const uint8 InActionOne = 0, const FBitWriter& InActionOneParams = FBitWriter(),
	           const uint8 InActionTwo = 0, const FBitWriter& InActionTwoParams = FBitWriter(),
	           const uint8 InActionThree = 0, const FBitWriter& InActionThreeParams = FBitWriter());

	//Returns false if the action was created on another frame.
	bool TryAddActionCheckIfSameFrame(const float InCurrentWorldTime, const uint8 InAction, const FBitWriter& InParams);

	bool operator==(const FActionSet& Other) const;

	bool operator!=(const FActionSet& Other) const;
	
	TMap<uint8, TBitArray<>> ToMap() const;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
	
	friend FArchive& operator<<(FArchive& Ar, FActionSet& Set)
	{
		Ar.SerializeBits(&Set.NumActionsMinusOne, 2);

		bool bSendParams = Set.ActionZeroParams.Num() > 0 ||
				Set.ActionOneParams.Num() > 0 ||
				Set.ActionTwoParams.Num() > 0 ||
				Set.ActionThreeParams.Num() > 0;

		Ar.SerializeBits(&bSendParams, 1);

		//Default action.
		Ar.SerializeBits(&Set.ActionZero, 6);
		if (bSendParams) Ar << Set.ActionZeroParams;
		if (bSendParams) Ar << Set.ActionZeroParams;

		//First overflow action.
		if (Set.NumActionsMinusOne > 0)
		{
			Ar.SerializeBits(&Set.ActionOne, 6);
			if (bSendParams) Ar << Set.ActionOneParams;
		}

		//Second overflow action.
		if (Set.NumActionsMinusOne > 1)
		{
			Ar.SerializeBits(&Set.ActionTwo, 6);
			if (bSendParams) Ar << Set.ActionTwoParams;
		}

		//Third overflow action.
		if (Set.NumActionsMinusOne > 2)
		{
			Ar.SerializeBits(&Set.ActionThree, 6);
			if (bSendParams) Ar << Set.ActionThreeParams;
		}
		return Ar;
	}
};

template<>
struct TStructOpsTypeTraits<FActionSet> : public TStructOpsTypeTraitsBase2<FActionSet>
{
	enum
	{
		WithNetSerializer = true,
		WithIdenticalViaEquality = true,
		WithCopy = true
	};
};

DECLARE_DYNAMIC_DELEGATE(FBufferedInputDelegate);

USTRUCT()
struct SFCORE_API FBufferedInput
{
	GENERATED_BODY()

	TArray<TSubclassOf<UCardObject>> OwnedCardsRequired;

	TArray<TSubclassOf<UCardObject>> OwnedCardsRequiredGone;

	TArray<TSubclassOf<UCardObject>> SharedCardsRequired;

	TArray<TSubclassOf<UCardObject>> SharedCardsRequiredGone;

	float LifetimePredictedTimestamp;

	FBufferedInputDelegate Delegate;

	FBufferedInput();

	FBufferedInput(const TArray<TSubclassOf<UCardObject>>& InOwnedCardsRequired,
	               const TArray<TSubclassOf<UCardObject>>& InOwnedCardsRequiredGone,
	               const TArray<TSubclassOf<UCardObject>>& InSharedCardsRequired,
	               const TArray<TSubclassOf<UCardObject>>& InSharedCardsRequiredGone,
	               const float InLifetimePredictedTimestamp, const FBufferedInputDelegate& InDelegate);

	bool operator==(const FBufferedInput& Other) const;

	bool CheckConditionsMet(const UConstituent* InCurrentConstituent) const;
};

template<>
struct TStructOpsTypeTraits<FBufferedInput> : public TStructOpsTypeTraitsBase2<FBufferedInput>
{
	enum
	{
		WithIdenticalViaEquality = true,
		WithCopy = true
	};
};

uint32 GetTypeHash(const FBufferedInput& BufferedInput);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FServer_OnExecute);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPredicted_OnExecute);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAutonomous_OnExecute);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSimulated_OnExecute);

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
 * Note: SF has special implementations for form based audiovisual effects in the plugin SfAudiovisual.
 * These should be used instead of directly producing these effects. The nodes are marked Predicted_,
 * Simulated_, and Autonomous_ (the last one is more rare since usually they are Predicted). They are designed to guarantee
 * that the effects work with complex networking scenarios such as rollback and relevancy.
 */
UCLASS(Blueprintable)
class SFCORE_API UConstituent : public USfObject
{
	GENERATED_BODY()

	friend class UFormCharacterComponent;

	friend class UInventory;

public:
	
	UConstituent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere)
	class USlotable* OwningSlotable;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Constituent")
	bool bEnableInputsAndPrediction = false;

	void ServerInitialize();

	void ServerDeinitialize();

	UFUNCTION(BlueprintImplementableEvent)
	void Server_Initialize(const bool bInIsPredictableContext = false);

	UFUNCTION(BlueprintImplementableEvent)
	void Server_Deinitialize(const bool bInIsPredictableContext = false);

	//Executes an action, which acts as an identifier for certain event graphs. See UConstituent description for details.
	//ActionId can only be between 1-63 inclusive.
	//Use ExecuteActionWithParams instead to pass params.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "bInIsPredictableContext"))
	void ExecuteAction(const int32 InActionId, const bool bInIsPredictableContext);
	
	//Executes an action, which acts as an identifier for certain event graphs. See UConstituent description for details.
	//ActionId can only be between 1-63 inclusive.
	//Use ExecuteAction if params aren't necessary.
	UFUNCTION(BlueprintCallable, CustomThunk, meta = (CustomStructureParam = "InActionParams", DefaultToSelf = "Target", AutoCreateRefTerm = "bInIsPredictableContext"))
	static void ExecuteActionWithParams(UConstituent* Target, const int32 InActionId, const bool bInIsPredictableContext, UStruct* InActionParams);

	DECLARE_FUNCTION(execExecuteActionWithParams)
	{
		P_GET_OBJECT(UConstituent, Target);
		P_GET_PROPERTY(FIntProperty, InActionId);
		P_GET_PROPERTY(FBoolProperty, bInIsPredictableContext);
		
		Stack.Step(Stack.Object, nullptr);

		//Grab the last property found walking the stack.
		//Contains type info and not value.
		FProperty* StructProperty = CastField<FProperty>(Stack.MostRecentProperty);

		//Ptr to value.
		void* StructPtr = Stack.MostRecentPropertyAddress;

		P_FINISH;

		FBitWriter Ar = FBitWriter(UINT8_MAX, true);
		
		SerializeGenericStruct(StructProperty, StructPtr, Ar);
		
		if (InActionId < 0 || InActionId > 63)
		{
			UE_LOG(LogSfCore, Error,
				   TEXT("ExecuteActionWithParams called with an out of range ActionId of %i on UConstituent class %s"), InActionId,
				   *Target->GetClass()->GetName());
			return;
		}
		Target->InternalExecuteAction(InActionId, bInIsPredictableContext, Ar);
	}
	
	void InternalExecuteAction(const uint8 InActionId, const bool bInIsPredictableContext, const FBitWriter& SerializedParams);

	//Does both serialization and deserialization based on archive direction.
	static void SerializeGenericStruct(FProperty* Property, void* StructPtr, FArchive& Ar);

	//Does both serialization and deserialization based on archive direction.
	static void SerializeProperty(FProperty* Property, void* ValuePtr, FArchive& Ar);

	//Called when an action is executed on the server.
	UPROPERTY(BlueprintAssignable)
	FServer_OnExecute Server_OnExecute;

	//Called when an action is executed on the server and client predictively.
	//Only use functions marked Predicted_.
	//bIsFirstPerson is only valid on the client.
	UPROPERTY(BlueprintAssignable)
	FPredicted_OnExecute Predicted_OnExecute;

	//Called when an action is executed on the autonomous client.
	UPROPERTY(BlueprintAssignable)
	FAutonomous_OnExecute Autonomous_OnExecute;

	//Called when an action is executed on the simulated client.
	UPROPERTY(BlueprintAssignable)
	FSimulated_OnExecute Simulated_OnExecute;
	
	void InternalServerOnExecute(const uint8 InActionId, const TBitArray<>& SerializedParams);
	
	void InternalPredictedOnExecute(const uint8 InActionId, const bool bInIsReplaying, const bool bInIsFirstPerson,
	                         const bool bInIsServer, const TBitArray<>& SerializedParams);
	
	void InternalAutonomousOnExecute(const uint8 InActionId, const float InTimeSinceExecution, const bool bInIsFirstPerson, const TBitArray<>& SerializedParams);
	
	void InternalSimulatedOnExecute(const uint8 InActionId, const float InTimeSinceExecution, const bool bInIsFirstPerson, const TBitArray<>& SerializedParams);

	//Input down for the input registered to the UConstituent in UFormCharacterComponent.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnInputDown(const bool bInIsPredictableContext);

	//Input up for the input registered to the UConstituent in UFormCharacterComponent.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnInputUp(const bool bInIsPredictableContext);

	//Opt in with bEnableLowFreqTick. Ticks at a rate set in UFormCoreComponent.
	UFUNCTION(BlueprintImplementableEvent)
	void Server_LowFrequencyTick(const float InDeltaTime, const bool bInIsPredictableContext = false);

	static bool IsIdWithinRange(const uint8 InId);

	UFUNCTION(BlueprintPure)
	int32 GetInstanceId() const;

	//Trace originating UConstituent until nullptr and then find the UFormCoreComponent.
	UFUNCTION(BlueprintPure)
	UConstituent* GetTrueOriginatingConstituent() const;

	//Get the constituent that created this UConstituent. Is nullable.
	UFUNCTION(BlueprintPure)
	UConstituent* GetOriginatingConstituent() const;

	UFUNCTION(NetMulticast, Unreliable)
	void NetMulticastClientPerformActionSet(const FActionSet& InActionSet, const float InServerTimestamp);
	
	//The last action set and time since values will have to be passed through as those would presumably not have updated.
	//This is so we can call internal client perform action set and play the most recent action(s).
	//This might be used to add post-relevancy action synchronization in the future, where actors are expected to go relevant and irrelevant
	//close to the line of sight of the viewer. But for the current use case RPC based actions should work well enough.
	void InternalClientPerformActionSet();

	void IncrementTimeSincePredictedLastActionSet(const float InTimePassed);

	void PerformLastActionSetOnClients();

	//Returns the query of QueryClass.
	//This needs to be casted to the class to get the event from the query.
	USfQuery* GetQuery(const TSubclassOf<USfQuery> QueryClass) const;

	//Buffers input until a certain card based condition is met. The timeout cannot be more than 1.5 seconds.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm =
		"InOwnedCardsRequiredToActivate, InOwnedCardsRequiredGoneToActivate, InSharedCardsRequiredToActivate, InSharedCardsRequiredGoneToActivate"
	))
	void BufferInput(const float InTimeout,
	                 const FBufferedInputDelegate& EventToBind,
	                 const TArray<TSubclassOf<UCardObject>>& InOwnedCardsRequiredToActivate,
	                 const TArray<TSubclassOf<UCardObject>>& InOwnedCardsRequiredGoneToActivate,
	                 const TArray<TSubclassOf<UCardObject>>& InSharedCardsRequiredToActivate,
	                 const TArray<TSubclassOf<UCardObject>>& InSharedCardsRequiredGoneToActivate);

	void InternalBufferInput(const float InTimeout,
	                         const FBufferedInputDelegate& EventToBind,
	                         const TArray<TSubclassOf<UCardObject>>& InOwnedCardsRequiredToActivate = TArray<TSubclassOf
		                         <UCardObject>>(),
	                         const TArray<TSubclassOf<UCardObject>>& InOwnedCardsRequiredGoneToActivate = TArray<
		                         TSubclassOf<UCardObject>>(),
	                         const TArray<TSubclassOf<UCardObject>>& InSharedCardsRequiredToActivate = TArray<
		                         TSubclassOf<UCardObject>>(),
	                         const TArray<TSubclassOf<UCardObject>>& InSharedCardsRequiredGoneToActivate = TArray<
		                         TSubclassOf<UCardObject>>());

	void HandleBufferInputTimeout();

	UFUNCTION(BlueprintPure)
	UFormCoreComponent* GetFormCoreComponent() const;

	//Unique identifier within each inventory.
	UPROPERTY(Replicated)
	uint8 InstanceId;

	//We use ActionSet instead of just uint8 for multiple reasons. Most importantly we need to do so as more than one action
	//can be executed within a frame. ActionSet allows us to work with up to four actions that execute in the same frame,
	//which should guarantee actions should not be skipped assuming the guidelines for creating them are followed. ActionSet
	//performs serialization with lossless compression and checks that only actions from the same frame are added.

	//This is changed during a predicted execution. Client version is sent to the server via FormCharacter, checked, added
	//to LastActionSet of that frame, and corrected to the client if necessary.
	FActionSet PredictedLastActionSet = FActionSet();

	//Used to know how long ago the last action took place so we can fast forward the effects after replay.
	FUint16_Quantize100 TimeSincePredictedLastActionSet = FUint16_Quantize100();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bEnableLowFreqTick = false;

	//Variables given to BP only during action execution through the K2Node_ConstituentAction.
	//Grabbed directly from the constituent class instead of passed through the delegate because
	//it's more complicated to find a way to make K2Nodes work with multicast delegates with different variables.

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly)
	int32 GetCurrentActionId() const;

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly)
	bool GetCurrentIsReplaying() const;

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly)
	bool GetCurrentIsFirstPerson() const;

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly)
	bool GetCurrentIsServer() const;

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly)
	float GetCurrentTimeSinceExecution() const;

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, CustomThunk, meta = (CustomStructureParam = "OutStruct", DefaultToSelf = "Target"))
	static void GetCurrentParams(UConstituent* Target, UStruct*& OutStruct);

	DECLARE_FUNCTION(execGetCurrentParams)
	{
		P_GET_OBJECT(UConstituent, Target);
		
		Stack.Step(Stack.Object, nullptr);

		//Grab the last property found walking the stack.
		//Contains type info and not value.
		FProperty* StructProperty = CastField<FProperty>(Stack.MostRecentProperty);

		//Ptr to value.
		void* StructPtr = Stack.MostRecentPropertyAddress;

		P_FINISH;
		
		SerializeGenericStruct(StructProperty, StructPtr, Target->CurrentBitReader);
	}
	
	int32 CurrentActionId;
	
	bool bCurrentIsReplaying;
	
	bool bCurrentIsFirstPerson;
	
	bool bCurrentIsServer;

	float CurrentTimeSinceExecution;

	TBitArray<> CurrentParams;

	FBitReader CurrentBitReader;

	//True to send and execute action set on clients.
	uint8 bLastActionSetPendingClientExecution:1;

protected:
	//USfQuery classes that this UConstituent depends on.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Constituent")
	TArray<TSubclassOf<USfQuery>> QueryDependencyClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Constituent")
	bool bQueryDependenciesAreOptional = false;
	
private:
	
	void SetFormCore();

	bool bAwaitingClientInit = true;

	//Actions performed on the last frame any actions were performed.
	FActionSet LastActionSet;

	//This is for replicating effects that started right before relevancy.
	//Uses replicated non-compensated timestamp found in form core.
	float LastActionSetTimestamp;

	UPROPERTY(Replicated)
	UFormCoreComponent* FormCore;

	UPROPERTY(Replicated)
	UConstituent* OriginatingConstituent;

	//In case the naming is confusing:
	//On the server, neither "time since" is relevant because actions are always executed instantly. On the simulated proxy,
	//we always use LastActionSet paired with LastActionSetTimestamp because prediction isn't running. On the autonomous proxy,
	//LastActionSetTimestamp will almost always be 0 since we're always relevant, so we only recreate the effects from
	//PredictedLastActionSet using the respective "time since" and let NetMulticastClientPerformActionSet handle LastActionSet changes.

	TSet<FBufferedInput> BufferedInputs;
};

TBitArray<> BitWriterToBitArray(const FBitWriter& Source);

void SetBitReader(FBitReader& Reader, TBitArray<>& Source);