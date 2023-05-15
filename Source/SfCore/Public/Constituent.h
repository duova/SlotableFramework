// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfObject.h"
#include "Constituent.generated.h"

USTRUCT(BlueprintType)
struct FConstituentStateData
{
	GENERATED_BODY()
	
	uint8 CurrentState;

	uint8 PreviousState;

	uint8 bClientCorrecting:1;

	FConstituentStateData();
	
	FConstituentStateData(uint8 CurrentState, uint8 PreviousState, bool bClientCorrecting);
};

/**
 * Building blocks of a slotable which can be reused to share functionality between slotables.
 * These are supposed to be scriptable in a blueprint class with targeters, operators, and events.
 *
 * ***Details***
 *
 * The activation state is an integer that represents a state. Conceptually every integer state represents a discrete
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
 * the designer, and finally calls the ChangeState blueprint function once conditions are reached. As a rule, state
 * drivers may not make any changes to the game state.
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
 * should be put into another state. Audiovisual effects obviously cannot be instantaneous, but they should be called
 * instantaneously.
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

	uint8 bIsBeingDestroyed:1;

	//Note: Slotables should async load all assets on init and unload on deinit.
	void ClientInitialize();

	void ServerInitialize();
	
	void ClientDeinitialize();

	void ServerDeinitialize();

private:
	UFUNCTION()
	void OnRep_OwningSlotable();

	uint8 bAwaitingClientInit:1;
};
