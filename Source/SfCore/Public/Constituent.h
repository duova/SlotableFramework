// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfObject.h"
#include "Constituent.generated.h"

USTRUCT(BlueprintType)
struct FSfPredictionFlags
{
	GENERATED_BODY()

	uint8 bClientStatesWereCorrected:1;
	uint8 bClientMetadataWasCorrected:1;
	uint8 bClientOnPlayback:1;
	uint8 bIsServer:1;

	FSfPredictionFlags(const bool bClientStatesWereCorrected, const bool bClientMetadataWasCorrected, const bool bClientOnPlayback, const bool bIsServer);
};

/**
 * Building blocks of a slotable which can be reused to share functionality between slotables.
 * These are supposed to be scriptable in a blueprint class with targeters, operators, and events.
 *
 * ***Details***
 *
 * The constituent state is an integer that represents a state. Conceptually every integer state represents a discrete
 * event that happens with the constituent. Examples of this are a spell being casted, a gun being fired, a reload
 * starting, a reload finishing, cooldown ending. Anything that happens or changes to the constituents properties should
 * be considered a state, however simultaneous events can be considered a singular state. For example, a spell being casted
 * and the cooldown starting are represented by entering the same state as they happen simultaneously. Implementing a
 * constituent in blueprints involves determining the flow of the state machine, then implementing the results of
 * entering each state. 
 *
 * ***State Driver***
 * 
 * The flow of the state machine is determined by a set of nodes referred to as the state driver. Blueprint events are
 * provided as inputs to a state driver, which are processed by nodes that check different conditions as desired by
 * the designer, and finally calls the ChangeConstituentState blueprint function once conditions are reached. As a rule,
 * state drivers may not make any changes to the game state directly.
 *
 * ***State Reactor***
 *
 * Note: FP is first person and TP is third person
 *
 * The results of entering each state is defined by another set of nodes referred to as the state reactor. This begins
 * with a blueprint event that indicates the new state that has been entered and which network role this state reactor
 * is a definition for. The server network role can use any node in order to produce any effects on the server, which
 * should be synchronized to the client if relevant. However, the simulated FP, simulated TP, and autonomous roles are
 * limited to nodes that are for presentation and not simulation. Four state reactors, one for each role, should be implemented
 * for each state that the constituent can enter. In addition, the predicted role, which executes on both the server and
 * autonomous client conditionally, can also be implemented only with nodes marked Predicted_. The autonomous role and
 * the predicted role do not have to be implemented for non-player forms. Game state condition checking should always be
 * done completely by the state driver before entering a state, so reactors should not have game state condition checks,
 * only presentation related checks. (eg. a stat check should be in the state driver, but a check of whether a UI section
 * is overflowing should be in the reactor) In addition, state reactions should be instantaneous, and delayed effects
 * should be put into another state (most Predicted_ nodes have a delay variable that can be used for short delays).
 * Audiovisual effects obviously cannot be instantaneous, but they should be called instantaneously.
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

	void Tick(float DeltaTime);
	
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
	
	//From is used to make a sanity check to prevent unexpected behaviour. It does not crash the game, it will only
	//make sure that an unexpected state change will consistently fail.
	UFUNCTION(BlueprintCallable)
	bool ChangeConstituentState(uint8 From, uint8 To);

	UFUNCTION(BlueprintImplementableEvent)
	void Server_OnEnterConstituentState(const uint8 State);

	//Only use functions marked Predicted_.
	UFUNCTION(BlueprintImplementableEvent)
	void Predicted_OnEnterConstituentState(const uint8 State, const FSfPredictionFlags PredictionFlags);

	//TimeSinceStateChange does not increment past 655 seconds.
	
	UFUNCTION(BlueprintImplementableEvent)
	void Autonomous_OnEnterConstituentState(const uint8 State, const float TimeSinceStateChange);

	UFUNCTION(BlueprintImplementableEvent)
	void SimulatedFP_OnEnterConstituentState(const uint8 State, const float TimeSinceStateChange);

	UFUNCTION(BlueprintImplementableEvent)
	void SimulatedTP_OnEnterConstituentState(const uint8 State, const float TimeSinceStateChange);
	
	UFUNCTION()
	void OnRep_ConstituentState();
	
private:
	UFUNCTION()
	void OnRep_OwningSlotable();

	uint8 bAwaitingClientInit:1;

	uint8 PredictedConstituentState;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_ConstituentState)
	uint8 ConstituentState;

	//Quantize100. Only mark dirty when ConstituentState is changed. This is for replicating effects that started right
	//before relevancy.
	UPROPERTY(Replicated)
	uint16 TimeSinceStateChange;
};
