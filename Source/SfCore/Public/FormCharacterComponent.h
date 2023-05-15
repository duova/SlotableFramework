// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FormCharacterComponent.generated.h"

	/* ***************************************************************
	 * 
	 * Slotable Framework Prediction (SFP)
	 * 
	 * ***************************************************************
	 *
	 * ***High Level Goals***
	 *
	 * The high level goals of SFP is similar to Unreal's Gameplay Ability Prediction, in that it also focuses on allowing
	 * designers to script abilities in blueprint while not having to pay much attention to prediction details.
	 * 
	 * However, since the Slotable Framework is a more genre specific implementation of abilities, it does not predict certain
	 * things that GAS does, while predicting some things that GAS does not. There are three tenets that SFP follows when
	 * deciding what should or should not be predicted and how it should be predicted:
	 *
	 * 1 - Prediction should be optimized for competitive integrity of a game built on top of the framework.
	 * 
	 * It is essential that cooldowns are predicted because it will affect the rate of fire of abilities. The expected
	 * ability cooldown is 500ms, where a 100ms delay to being able to use an ability is absurd. Focusing on competitive
	 * integrity also means that rollback should have the least disruption to gameplay possible. This means visual
	 * rollback smoothing should be short if even implemented.
	 * 
	 * 2 - Balance between the audiovisual obviousness and prediction complexity.
	 *
	 * SFP does not intend to predict any change where the variables that will be dirtied are difficult or impossible to
	 * trace. For example, the addition of status effects (which should be implemented as a slotable), as the visual
	 * changes for the client are minimal (likely to be HUD changes where a 200ms delay will not be very noticeable).
	 * It is also highly complex to predict as in the SF model, slotable addition is not particularly limited in its
	 * effects as it is blueprint scripted. The variables dirtied as a result of prediction cannot be effectively
	 * tracked for rollback, and a specific rollback implementation must be created for each case.
	 *
	 * 3 - The preferred manner of prediction is one where a predicted audiovisual effect is used to cover up the latency
	 * of a state change where the costs of potential rollback are too high for the change to be predicted.
	 * 
	 * Audiovisual effects (animations, VFX, SFX) are the focus of SFP as they have a huge impact on game feel, and SFP
	 * has a purpose built set of features that attempts to make predicting these effects as straightforward as possible
	 * for designers. As a continuation of the status effect case, it is possible to simply predict an audiovisual effect
	 * that is executed with the addition of a status effect/slotable in order to cover up the latency. (eg. The delay of
	 * a status effect that causes stat changes, HUD changes, and mana changes will be registered by the player simply
	 * as casting time if there is a predicted visual effect that is instantly started on activation.) This also goes for
	 * objects spawned by slotables.
	 *
	 * ***Implementation Details***
	 *
	 * Note: This part assumes an understanding of the comments in Constituent.
	 * 
	 * SFP builds on top of the existing CMC prediction implementation for the sake of not writing another prediction
	 * implementation from scratch. As such, the codebase might not read logically as SFP is using a movement
	 * prediction system to perform the prediction of other systems, which is not the CMCs intended function. The SFP is
	 * designed to guard the user against non-rollback safe prediction logic assuming the guidelines are followed.
	 *
	 * This requires PackedMovementRPCs to work.
	 *
	 * We override the CMC, FCharacterNetworkMoveDataContainer, FCharacterNetworkMoveData, and FCharacterMoveResponseDataContainer
	 * as well as a few functions to pass custom inputs (from slotables) through the CMC, have the server validate the
	 * custom predictive changes, and pass relevant custom state data back to the client. ie. As far as the CMC is concerned,
	 * we treat all inputs like movement inputs, and constituent state like movement state. PerformMovement is overriden to
	 * call the relevant blueprint input events on relevant constituents. Assuming a correct constituent BP class setup,
	 * this should drive changes in the activation state. This should happen on both the autonomous proxy and the
	 * authority where the PerformMovement function is called on. The activation state change should run the
	 * predicted state reactor on the autonomous client. These call specific functions marked Predicted_ that will
	 * perform predicted actions differently based on the function being predicted. Specific node implementations are outlined
	 * below. If the server corrects the state (which includes both the state and time since entering the state),
	 * PerformMovement will be called again by the CMC to playback and get the new predicted state, after which each
	 * predicted action implementation will detect that the state has changed and create the expected effects at that state
	 * and time. When state changes on the server, an RPC is simply called to run the autonomous and simulated state reactors
	 * on the relevant clients.
	 *
	 * Cooldown & Timers
	 *
	 * Server_PredictiveTimer nodes offer timer prediction to state reactors. These are supposed to be called on the server
	 * state reactor and uses a UENUM as a cross-network identifier. These timers call an event with the identifier when
	 * the timer ends. There is also a function can be called to check if a timer has ended and how much time is remaining
	 * if it has not. They also have modification functions.
	 * Internally, the way that these timers work is that they will only set on the server, and their creation, deletion
	 * and modification is RPCed to the client. However, these timers are timestamp based and not delta time based. The
	 * clock they rely on is generated by incrementing a float by DeltaTime in PerformMovement. This float is then treated
	 * as a movement variable in CMC and be synchronized and checked similarly to the rest of the movement variables. This
	 * guarantees that as long as the prediction ack tolerance is less than the server finish threshold of the timer, it
	 * should rarely mispredict an ability due to the timer. The timer should also work during playback, as it is incremented
	 * and checks are made inside PerformMove, so timer reliant state changes can still function properly with prediction.
	 *
	 * Movement Operators
	 *
	 * Predicted_Impulse and Predicted_Force are simply called in PerformMove. Predicted_VelocityOverTime and
	 * Predicted_ForceOverTime, which can disable movement and will lock the character into a velocity/acceleration for
	 * a period of time, uses the PredictiveTimer to determine when to stop the movement.
	 *
	 * FX
	 *
	 * Whenever Predicted_ audio, animation, and vfx are called, we check whether it is part of playback or an actual
	 * tick. If it is part of a playback, we save the given clip and wait until the next actual tick, where we play the
	 * saved clip starting from the timestamp copied from TimeSinceStateChange. Otherwise, we simply play the clip. Some
	 * smoothing features may be implemented to reduce how jarring the rollback is.
	 * Material changes can also be predicted. It has a similar implementation to the above but instead of playing clips,
	 * the provided nodes are wrappers for set parameter on material functions. They offer curves to set how the material
	 * parameters should change.
	 * Two custom anim notifies are added. First there is the anim notify equivalent of the material change nodes. Second
	 * there is a notify that spawns actors on sockets, with a curve to set the position and rotation of them over time.
	 * These lightweight actors are given a value for time every tick, and the implementer is expected to produce effects
	 * based on that time value, so that the sequence of actions can start in the middle. Both of these are designed for
	 * rollback.
	 *
	 * Projectiles, Hitscans, and Hit Effects
	 *
	 * Projectiles are predicted in order to keep consistency in the game - so players are not expected to lead their
	 * projectiles more or less based on ping. The autonomous client will fire the projectile and simulate instantly when
	 * they are supposed to fired the projectile. When the fire event gets to the server, we fire from where the autonomous
	 * client is on the server, but we forward the trajectory by half-RTT, so that in real time, the projectile is in the
	 * same place on the autonomous client and on the server. However, the autonomous client fired at a simulated client
	 * that is delayed by RTT, whereas on the server, the target is only delayed by half-RTT compared to the autonomous
	 * client. Therefore, we still need to perform lag compensation and check the projectile's trajectory against colliders
	 * half-RTT back in time. This prediction model favors the autonomous client. For everyone else, they see a projectile
	 * being spawned projectile velocity * half-RTT in front of where it should be, and they also are lag compensated by
	 * half-RTT.
	 * To make sure the target player is relatively content with this model, we can interpolate the initial spawning on
	 * the non-shooter clients to make it look like a fast projectile was slowed down, rather than a projectile being
	 * spawned in front of the shooter. The idea of this model is that we make half of the latency visible to the player,
	 * so that they can better react to it, but lag compensate the rest so that it does not seem incredibly obvious. Unlike
	 * some other projectile prediction models, this one does not need exceptions for rewinding (for reactive ability use)
	 * as only the target's collider needs to be rewound, not their state. This is because the projectile on the server
	 * is actually accurate to the autonomous client, and there is no need to lag compensate for time, only location as
	 * the autonomous client fired at a half-RTT movement late target. What this means is that the autonomous client can
	 * only mispredict a landed hit by half-RTT, and they will instantly see why afterwards. For the owning client of the
	 * target, if their reactive ability (of some form of invulnerability for example) is registered first on the server,
	 * they cannot get hit even if rewound.
	 * Since projectiles have a predictable trajectory, if we replay and realise that we've fired a projectile on the client,
	 * we can simply spawn the projectile ahead in time to make sure the server projectile is represented on the client.
	 * Hitscans/line traces are to be lag compensated/rewound in location by RTT. This favors the autonomous client in
	 * terms of actually hitting the shot, but favors the target in using a state change to counteract the effects of the
	 * shot.
	 * Basic hit effects are to be produced by the predicted projectile hit. Major hit effects and actual state changes
	 * are to be produced on confirmation.
	 * Projectiles that are either very slow or are not directly aimed by the autonomous client should not be predicted.
	 * 
	 * Slotable & Card Changes
	 *
	 * We do not fully predict slotable changes because slotables are capable of producing non-rollback safe effects. We
	 * do, however, want to know what slotables might have been added as certain other slotables might be dependent on
	 * their addition. For example, we have an ability that grant a certain buff to the player, and another that can only
	 * be activated if the player has that buff. We would want the player to be able to activate the second ability instantly
	 * after they have activated the first, which means the buff (a slotable) must be in some way added as a prediction.
	 * To do so we keep a list of classes on each inventory to represent the slotables that should be in the
	 * inventory. These classes can be repeated. Instead of adding and removing slotables, we add and remove these classes
	 * when slotable changes are predicted to happen on the autonomous client (by Predicted_ functions). These classes are
	 * replicated and will correct the client for any missing additions or deletions, while the client will be responsible
	 * for realising the state that brought about a change was rolled back and revert predicted changes.
	 * Card changes on the other hand have relatively deterministic behaviour, and as such can be fully predicted.
	 *
	 * Movement Speed Change
	 *
	 * Movement speed must be predicted as it is a movement variable and will make the CMC issue corrections every time
	 * it is changed if not. Therefore, it cannot be treated like other stats, yet it still needs to be controlled by
	 * slotables. As such, every normal slotable is simply given a +/- multiplicative/additive movement speed modifier,
	 * which is applied by the slotable change functions to the CMC movement speed variable automatically. (These are
	 * Predicted_)
	 */

class FSavedMove_Sf : public FSavedMove_Character
{
public:
	FSavedMove_Sf();
	
	typedef FSavedMove_Character Super;
	
	//Movement inputs.
	uint8 bWantsToSprint:1;

	//Number of input sets to enable.
	uint8 EnabledInputSets:2;
	
	//Additional inputs.
	uint8 PrimaryInputSet;
	uint8 SecondaryInputSet;
	uint8 TertiaryInputSet;

	float PredictedNetClock;
	
	TArray<uint8> ConstituentStates;

	TArray<TSubclassOf<UObject>> MetadataClasses;

	virtual void Clear() override;
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel,
		FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* C) override;
	virtual uint8 GetCompressedFlags() const override;
};

class FNetworkPredictionData_Client_Sf : public FNetworkPredictionData_Client_Character
{
public:
	explicit FNetworkPredictionData_Client_Sf(const UCharacterMovementComponent& ClientMovement);

	typedef FNetworkPredictionData_Client_Character Super;

	virtual FSavedMovePtr AllocateNewMove() override;
};

struct FSfNetworkMoveData : FCharacterNetworkMoveData
{
	//Number of input sets to enable.
	uint8 EnabledInputSets:2;
	
	//Additional inputs.
	uint8 PrimaryInputSet;
	uint8 SecondaryInputSet;
	uint8 TertiaryInputSet;

	float PredictedNetClock;
	
	TArray<uint8> ConstituentStates;
	
	TArray<TSubclassOf<UObject>> MetadataClasses;
	
	FSfNetworkMoveData();

	virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType) override;

	virtual void ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType) override;
};

struct FSfNetworkMoveDataContainer : FCharacterNetworkMoveDataContainer
{
	FSfNetworkMoveData NewSfMove;
	FSfNetworkMoveData PendingSfMove;
	FSfNetworkMoveData OldSfMove;

	FSfNetworkMoveDataContainer();
};

struct FSfMoveResponseDataContainer : FCharacterMoveResponseDataContainer
{
	TArray<uint8> ConstituentStates;

	float PredictedNetClock;

	//Quantize100
	TArray<uint16> TimeSinceStateChange;

	TArray<FInventoryObjectMetadata> Metadata;

	FSfMoveResponseDataContainer();

	virtual void ServerFillResponseData(const UCharacterMovementComponent& CharacterMovement, const FClientAdjustment& PendingAdjustment) override;

	virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap) override;
};

/**
* The FormCharacterComponent is responsible for the movement and client prediction of a form.
* The form must be an ACharacter. This component must be replicated.
* The component should be used for all forms that may be player controlled or require CharacterMovementComponent features.
* AI controllers and player controllers should write inputs to this component.
* This component should not be used if the form will never be player controlled and does not need character movement.
* In that case, the controllers should directly call functions on FormBaseComponent instead of writing inputs.
* This component extends the CharacterMovementComponent to use its prediction logic to drive prediction for slotables.
*/
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SFCORE_API UFormCharacterComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UFormCharacterComponent();

	virtual bool ShouldUsePackedMovementRPCs() const override;

	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	virtual void ClientAdjustPosition_Implementation(float TimeStamp, FVector NewLoc, FVector NewVel, UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode, TOptional<FRotator> OptionalRotation) override;

	//Movement inputs.
	uint8 bWantsToSprint:1;

	//Number of input sets to enable.
	uint8 EnabledInputSets:2;
	
	//Additional inputs.
	uint8 PrimaryInputSet;
	uint8 SecondaryInputSet;
	uint8 TertiaryInputSet;

	//This is obviously not optimal as we are already sending the client TimeStamp. However, this is the most direct way
	//of ensuring that time discrepancies will not cause more rollbacks - by having the clock incremented in tandem with
	//the logic that uses it in PerformMovement. To save bandwidth we only serialize this to the server once every second,
	//and the server will only issue a correction when necessary.
	float PredictedNetClock;
	//TODO: RPC TO RESET THIS VALUE WHEN CLOCK HITS MAX VALUE
	uint32 NetClockNextInteger;

	//+/- acceptable range.
	UPROPERTY(EditAnywhere)
	float NetClockAcceptableTolerance;

	//We will only serialize to server on change.
	//TODO: BELOW
	//An expected issue is that a rollback will occur every time we add/remove a constituent as there will be a state desync.
	//To solve this, we mark a constituent as being destroyed if we predict its destruction on the client. The iterator
	//that collects the states will then skip the constituent to maintain a deterministic state order even if the client's
	//constituent has not actually been destroyed yet. We do this on the server too in case the object is accessed before
	//it is destroyed.
	//For constituent creation, we preemptively insert a 0 state to where we believe a constituent will be created. It will
	//still rollback if the server initializes the constituent with a non-zero state, but in that case we will have to rollback
	//anyway to predict the state.
	TArray<uint8> ConstituentStates;
	TArray<uint8> OldConstituentStates;
	//Quantize100
	TArray<uint16> TimeSinceStateChange;

	//If true on client, normal tick should call a delegate before reverting to false.
	uint8 bClientStatesWereCorrected:1;

	//Ordered. MetadataClasses should be updated when changed.
	TArray<TSubclassOf<UObject>> MetadataClasses;
	TArray<TSubclassOf<UObject>> OldMetadataClasses;

	//Ordered. Updated by server when a correction is needed, but otherwise not updated.
	//Should only be used when bClientMetadataWasCorrected is true.
	//Updated on server by form core.
	TArray<FInventoryObjectMetadata> Metadata;

	//If true on client, normal tick should call a delegate before reverting to false.
	uint8 bClientMetadataWasCorrected:1;
	
protected:
	virtual void BeginPlay() override;

	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	virtual bool ServerCheckClientError(float ClientTimeStamp, float DeltaTime, const FVector& Accel, const FVector& ClientWorldLocation, const FVector& RelativeClientLocation, UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName, uint8 ClientMovementMode) override;

	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel) override;
	
	virtual bool SfServerCheckClientError();

	virtual void UpdateFromAdditionInputs();

private:
	FSfNetworkMoveDataContainer SfNetworkMoveDataContainer;

	FSfMoveResponseDataContainer SfMoveResponseDataContainer;
};