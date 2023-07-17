// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Constituent.h"
#include "Card.h"
#include "EnhancedInputComponent.h"
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
 * Note: This part assumes an understanding of the comments in Constituent and Card.
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
 * custom predictive changes, and pass relevant custom action state data back to the client. ie. As far as the CMC is concerned,
 * we treat all inputs like movement inputs, and action state like movement state. PerformMovement is overriden to
 * call the relevant blueprint input events on relevant constituents. Assuming a correct constituent BP class setup,
 * this should drive action execution. This should happen on both the autonomous proxy and the authority where the
 * PerformMovement function is called on. The action condition should run the predicted execution graph on the autonomous
 * client. These call specific functions marked Predicted_ that will perform predicted actions differently based on
 * the function being predicted. Specific node implementations are outlined below. If the server corrects the action state
 * (which includes both the last action and time since executing the action), PerformMovement will be called again
 * by the CMC to playback and get the new predicted state, after which each predicted action implementation will detect
 * that the state has changed and create the expected effects after the given action at the given time. This depends on
 * stateless implementations of Predicted_ nodes. When an action executes on the server, an OnRep function simply causes
 * the autonomous and simulated clients to execute their respective graphs.
 *
 * Card Changes
 *
 * Slotable changes are not predicted due to the complexity that comes with a blueprintable replicated UObject (the
 * engine's network object creation logic can't be overriden). However, cards can be predicted and for 99% of cases
 * that will be enough to achieve the same effect. Card changes are simply cycled through the FormCharacter/CMC prediction
 * cycle. The difficulty comes from the fact that they can also be server created and destroyed. In order to prevent
 * excessive rollbacks whenever there is a change on the server but not predicted on the client, we mark cards to not
 * use correction / is disabled due to destruction. Marking them as not using correction will prevent the prediction
 * checks from thinking that the server made a change while the client has not. While marking them as is disabled will
 * prevent it from being used, while still allowing the prediction checks to think it exists. When the client syncs up,
 * these changes will be finalized by having correction work normally again and properly destroying the card on the
 * server for the two cases respectively. If the client doesn't sync up, we have a timeout to take the card out of the
 * temporary state anyway so the client can receive the changes again and rollback. Card prediction allows predicted
 * markers to be added.
 *
 * Card Lifetime
 *
  * Cards have lifetimes that are adaptively predictive or server based. This is so they can be used as predicted timers
 * or just general network synced timers. They use timestamps and are relatively light on bandwidth. When lifetime ends,
 * an event can be called.
 * Internally, for predicted lifetime, the clock they rely on is generated by incrementing a float by DeltaTime in PerformMovement.
 * This float is then treated as a movement variable in CMC and be synchronized and checked similarly to the rest of
 * the movement variables. This guarantees that as long as the prediction ack tolerance is less than the server finish
 * threshold of the lifetime, it should rarely mispredict an ability. The lifetime should also work during playback,
 * as it is incremented and checks are made inside PerformMove, so lifetime reliant actions can still function
 * properly with prediction.
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
 * saved clip starting from the timestamp copied from TimeSinceLastActionSet. Otherwise, we simply play the clip. Some
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
 * Movement Speed Change
 *
 * Movement speed must be predicted as it is a movement variable and will make the CMC issue corrections every time
 * it is changed if not. As such, every card is simply given a +/- multiplicative/additive movement speed modifier,
 * which is applied by the card change functions to the CMC movement speed variable automatically.
 *
 * TODO: Delay should be available on all nodes to ensure post relevancy accuracy.
 * Delay
 * 
 * Most Predicted_ nodes have a delay float that will delay the execution of the node by the given time. This is to
 * allow for delaying functionality by allowing the Predicted_ implementation to know about the delay, so it can account
 * for it during playback. For example, if the state changed 100ms before playback finished, then a FX node will start
 * the expected effects 100ms late. However, this would not work if the effect was in fact delayed by a non-prediction
 * safe timer node, altering the starting time.
 */

class UInventory;
struct FCard;

USTRUCT()
struct SFCORE_API FInventoryCards
{
	GENERATED_BODY()
	
	TArray<FCard> Cards;

	FInventoryCards();

	explicit FInventoryCards(const TArray<FCard>& InCards);

	bool operator==(const FInventoryCards& Other) const;

	friend FArchive& operator<<(FArchive& Ar, FInventoryCards& InventoryCards)
	{
		Ar << InventoryCards.Cards;
		return Ar;
	}
};

USTRUCT()
struct SFCORE_API FNetCardIdentifier
{
	GENERATED_BODY()
	
	uint16 ClassIndex;
	
	uint8 OwnerConstituentInstanceId;

	FNetCardIdentifier();

	FNetCardIdentifier(uint16 InClassIndex, uint8 InOwnerConstituentInstanceId);

	bool operator==(const FNetCardIdentifier& Other) const;

	friend FArchive& operator<<(FArchive& Ar, FNetCardIdentifier& CardIdentifier)
	{
		//We try to not send the full index if we don't have to.
		bool bClassIndexIsLarge = CardIdentifier.ClassIndex > 255;
		Ar.SerializeBits(&bClassIndexIsLarge, 1);
		if (bClassIndexIsLarge)
		{
			Ar << CardIdentifier.ClassIndex;
		}
		else
		{
			uint8 Value = CardIdentifier.ClassIndex;
			Ar << Value;
			CardIdentifier.ClassIndex = Value;
		}
		Ar << CardIdentifier.OwnerConstituentInstanceId;
		return Ar;
	}
};

USTRUCT()
struct SFCORE_API FInventoryCardIdentifiers
{
	GENERATED_BODY()
	
	TArray<FNetCardIdentifier> CardIdentifiers;

	FInventoryCardIdentifiers();

	explicit FInventoryCardIdentifiers(const TArray<FNetCardIdentifier>& InCardIdentifiers);

	bool operator==(const FInventoryCardIdentifiers& Other) const;

	friend FArchive& operator<<(FArchive& Ar, FInventoryCardIdentifiers& InventoryCardIdentifiers)
	{
		Ar << InventoryCardIdentifiers.CardIdentifiers;
		return Ar;
	}
};

USTRUCT()
struct SFCORE_API FIdentifiedActionSet
{
	GENERATED_BODY()
	
	uint16 ConstituentInstanceId;
	
	FActionSet ActionSet;

	FIdentifiedActionSet();

	FIdentifiedActionSet(uint16 InConstituentInstanceId, FActionSet InActionSet);

	bool operator==(const FIdentifiedActionSet& Other) const;

	friend FArchive& operator<<(FArchive& Ar, FIdentifiedActionSet& ConstituentActionSet)
	{
		Ar << ConstituentActionSet.ConstituentInstanceId;
		Ar << ConstituentActionSet.ActionSet;
		return Ar;
	}
};

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

	TArray<FIdentifiedActionSet> PendingActionSets;

	TArray<FInventoryCardIdentifiers> InventoryCardIdentifiers;

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

	TArray<FIdentifiedActionSet> PendingActionSets;

	TArray<FInventoryCardIdentifiers> InventoryCardIdentifiers;

	FSfNetworkMoveData();

	virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap,
	                       ENetworkMoveType MoveType) override;

	virtual void ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType) override;
};

enum ECorrectionConditionFlags : uint8
{
	Repredict_Movement = 0x01,
	Repredict_NetClock = 0x02,
	//We must repredict both to repredict one because they are reliant on each other.
	Repredict_ActionSetsAndCards = 0x04,
	//This overrides the update flag as if we rollback we guarantee that we send server state anyway.
	Update_Cards = 0x08 //If update we only want to respond with changes and not rollback cards specifically.
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
	TArray<FActionSet> ActionSetResponse;

	float PredictedNetClock;

	TArray<FUint16_Quantize100> TimeSinceLastAction;

	TArray<FInventoryCards> CardResponse;

	FSfMoveResponseDataContainer();

	virtual void ServerFillResponseData(const UCharacterMovementComponent& CharacterMovement,
	                                    const FClientAdjustment& PendingAdjustment) override;

	void SerializeCardResponse(FArchive& Ar, UFormCharacterComponent* CharacterComponent, bool bIsSaving);

	virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar,
	                       UPackageMap* PackageMap) override;
};


DECLARE_MULTICAST_DELEGATE(FOnPostRollback);

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

	virtual void ClientAdjustPosition_Implementation(float TimeStamp, FVector NewLoc, FVector NewVel,
	                                                 UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase,
	                                                 bool bBaseRelativePosition, uint8 ServerMovementMode,
	                                                 TOptional<FRotator> OptionalRotation) override;

	//Marking cards dirty allows them to be synchronized to a predicting client without rollback.
	virtual void MarkCardsDirty();

	bool IsReplaying() const;

	void SetupFormCharacter(UFormCoreComponent* FormCoreComponent);

	//Movement inputs.
	uint8 bWantsToSprint:1;

	//Number of input sets to enable, this is automatically set based on how many inputs are registered.
	uint8 EnabledInputSets:2;

	//Additional inputs.
	uint8 PrimaryInputSet;
	uint8 SecondaryInputSet;
	uint8 TertiaryInputSet;

	//This is used to determine how the pipeline is supposed to handle the separate sections of correction data to rollback
	//as little as possible and send as little as possible.
	uint8 CorrectionConditionFlags; //Only first 4 bits is used.

	//This is obviously not optimal as we are already sending the client TimeStamp. However, this is the most direct way
	//of ensuring that time discrepancies will not cause more rollbacks - by having the clock incremented in tandem with
	//the logic that uses it in PerformMovement. To save bandwidth we only serialize this to the server once every second,
	//and the server will only issue a correction when necessary.
	float PredictedNetClock;
	uint32 NetClockNextInteger;
	
	float CalculateFuturePredictedTimestamp(const float AdditionalTime) const;

	float CalculateTimeUntilPredictedTimestamp(const float Timestamp) const;

	float CalculateTimeSincePredictedTimestamp(const float Timestamp) const;

	bool HasPredictedTimestampPassed(const float Timestamp) const;
	
	//+/- acceptable range.
	const float NetClockAcceptableTolerance = 0.02f;

	//Actions executed in a given frame that we send to the server for checking.
	//Note that this is representative of PredictedLastActionSet in Constituent and not LastActionSet.
	//This tracks the delta, only action sets that have been changed in the frame will be added.
	//Only checking the changes is feasible here because actions are relatively stateless. Also cards will force a correction
	//and rollback for actions if actions spawn cards that desync.
	TArray<FIdentifiedActionSet> PendingActionSets;

	//Server corrects with last action set for all constituents in order.
	TArray<FActionSet> ActionSetResponse;

	//The execution graph of the last action is called and then "fast forwarded" by this time to ensure that even if
	//another execution does not happen during replay, we still have the correct time on the previous execution.
	TArray<FUint16_Quantize100> TimeSinceLastAction;

	//If this is false, we shouldn't use ActionSetResponse.
	//We copy ActionSetResponse to the constituents and set false in PerformMovement if is true.
	uint8 bClientActionsWereUpdated:1;

	//Simplified versions of cards used to verify that client cards are correct.
	TArray<FInventoryCardIdentifiers> InventoryCardIdentifiers;
	TArray<FInventoryCardIdentifiers> OldInventoryCardIdentifiers;

	TArray<FInventoryCards> CardResponse;

	//If this is false, we shouldn't use CardResponse.
	//We copy CardResponse to the inventory and set false in PerformMovement if is true.
	uint8 bClientCardsWereUpdated:1;

	FOnPostRollback OnPostRollback;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<UInputAction*> InputActionRegistry;

	void OnInputDown(const FInputActionInstance& Instance);

	void OnInputUp(const FInputActionInstance& Instance);

protected:
	virtual void BeginPlay() override;

	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	virtual bool ServerCheckClientError(float ClientTimeStamp, float DeltaTime, const FVector& Accel,
	                                    const FVector& ClientWorldLocation, const FVector& RelativeClientLocation,
	                                    UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName,
	                                    uint8 ClientMovementMode) override;

	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags,
	                            const FVector& NewAccel) override;

	bool CardHasEquivalentCardIdentifier(const FInventoryCardIdentifiers& CardIdentifierInventory, const FCard& Card);

	bool CardCanBeFoundInInventory(UInventory* Inventory, FNetCardIdentifier CardIdentifier);

	void HandleInventoryDifferencesAndSetCorrectionFlags(UInventory* Inventory,
	                                                     const FInventoryCardIdentifiers& CardIdentifierInventory);

	virtual void SfServerCheckClientError();

	virtual void UpdateFromAdditionInputs();

	virtual bool ClientUpdatePositionAfterServerUpdate() override;

	void CorrectActionSets();

	void CorrectCards();

	void PackActionSets();

	void PackCards();

	void PackInventoryCardIdentifiers();

	void HandleCardClientSyncTimeout() const;

	static void ApplyInputBitsToInventory(uint32 InputBits, const UInventory* Inventory);
	
	void RemovePredictedCardWithEndedLifetimes();

	//This is where all the logic actually takes place.
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;

private:
	FSfNetworkMoveDataContainer SfNetworkMoveDataContainer;

	FSfMoveResponseDataContainer SfMoveResponseDataContainer;

	UPROPERTY()
	UFormCoreComponent* FormCore;

	UPROPERTY()
	UEnhancedInputComponent* EnhancedInputComponent;

	uint8 FindInputIndexInRegistry(const FInputActionInstance& Instance);

	//Sets a value on an input set using an index from 0-7.
	static void SetInputSet(uint8* InputSet, const uint8 Index, const bool bIsTrue);

	//Gets a value from all input sets using the input index from 0-23.
	static bool GetValueFromInputSets(const uint8 Index, const uint32 InputSetsJoinedBits);

	//Gets the bits of the input sets as one value.
	uint32 GetInputSetsJoinedBits() const;
};
