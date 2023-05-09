// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
	 * rollback smoothing should be short if even implemented. Projectiles and spawned objects should not be predicted
	 * as it is jarring and the predicted projectile will not be an accurate representation (likely that it can miss while
	 * looking like it hit). Predicted visual effects (which in most games last a few hundred ms) should be enough to
	 * mitigate the spawn delay felt under reasonable circumstances.
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
	 * as casting time if there is a predicted visual effect that is instantly started on activation.)
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
	 * Cooldown
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
	 * 
	 * Slotable & Tag Slotable Changes
	 *
	 * We do not fully predict slotable changes because slotables are capable of producing non-rollback safe effects. We
	 * do, however, want to know what slotables might have been added as certain other slotables might be dependent on
	 * their addition. For example, we have an ability that grant a certain buff to the player, and another that can only
	 * be activated if the player has that buff. We would want the player to be able to activate the second ability instantly
	 * after they have activated the first, which means the buff (a slotable) must be in some way added as a prediction.
	 * To do so we keep a list of gameplay tags on each inventory to represent the slotables that should be in the
	 * inventory. These tags can be repeated. Instead of adding and removing slotables, we add and remove these tags
	 * when slotable changes are predicted to happen on the autonomous client (by Predicted_ functions). These tags are
	 * replicated and will correct the client ones when the server makes these slotable changes.
	 *
	 * Movement Speed Change
	 *
	 * Movement speed must be predicted as it is a movement variable and will make the CMC issue corrections every time
	 * it is changed if not. Therefore, it cannot be treated like other stats, yet it still needs to be controlled by
	 * slotables. As such, every normal slotable is simply given a +/- multiplicative/additive movement speed modifier,
	 * which is applied by the slotable change functions to the CMC movement speed variable automatically. (These are
	 * Predicted_)
	 */


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
	// Sets default values for this component's properties
	UFormCharacterComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
