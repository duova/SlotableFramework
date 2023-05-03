// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfObject.h"
#include "Slotable.h"
#include "Constituent.generated.h"

/**
 * Building blocks of a slotable which can be reused to share functionality between slotables.
 * These are supposed to be scriptable in a blueprint class with targeters, operators, and events.
 */
UCLASS(Blueprintable)
class SFCORE_API UConstituent : public USfObject
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere)
	USlotable* OwningSlotable;

	void Initialize();

	void Deinitialize();

private:
	/**
	 * ***************************************************************
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
	 * ability cooldown is 500ms, where a 200ms delay to being able to use an ability is absurd. Focusing on competitive
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
	 * of a state change where the costs of potential rollback too high for the change to be predicted.
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
	 */
};
