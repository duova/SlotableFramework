// Fill out your copyright notice in the Description page of Project Settings.


#include "Constituent.h"

#include "FormCharacterComponent.h"
#include "FormCoreComponent.h"
#include "Net/UnrealNetwork.h"

FSfPredictionFlags::FSfPredictionFlags(): bClientPredictedLastActionSetWasCorrected(0), bClientMetadataWasCorrected(0),
                                          bClientOnPlayback(0), bIsServer(0)
{
}

FSfPredictionFlags::FSfPredictionFlags(const bool bClientPredictedLastActionSetWasCorrected,
                                       const bool bClientMetadataWasCorrected,
                                       const bool bClientOnPlayback, const bool bIsServer):
	bClientPredictedLastActionSetWasCorrected(bClientPredictedLastActionSetWasCorrected),
	bClientMetadataWasCorrected(bClientMetadataWasCorrected),
	bClientOnPlayback(bClientOnPlayback),
	bIsServer(bIsServer)
{
}

FActionSet::FActionSet(): NumActionsIncludingZero(0), ActionZero(0), ActionOne(0), ActionTwo(0), ActionThree(0),
                          WorldTime(0)
{
}

FActionSet::FActionSet(const float CurrentWorldTime, const uint8 ActionZero, const uint8 ActionOne,
                       const uint8 ActionTwo, const uint8 ActionThree): NumActionsIncludingZero(0),
                                                                        ActionZero(ActionZero), ActionOne(ActionOne),
                                                                        ActionTwo(ActionTwo),
                                                                        ActionThree(ActionThree),
                                                                        WorldTime(CurrentWorldTime)
{
	checkf(ActionZero != 0, TEXT("Created ActionSet with no action zero."));
	UConstituent::ErrorIfIdNotWithinRange(ActionZero);
	if (ActionOne != 0)
	{
		NumActionsIncludingZero = 1;
		UConstituent::ErrorIfIdNotWithinRange(ActionOne);
	}
	if (ActionTwo != 0)
	{
		NumActionsIncludingZero = 2;
		checkf(ActionOne != 0, TEXT("Created ActionSet with action two but no action one."));
		UConstituent::ErrorIfIdNotWithinRange(ActionTwo);
	}
	if (ActionThree != 0)
	{
		NumActionsIncludingZero = 3;
		checkf(ActionTwo != 0, TEXT("Created ActionSet with action three but no action two."));
		UConstituent::ErrorIfIdNotWithinRange(ActionThree);
	}
}

bool FActionSet::TryAddActionCheckIfSameFrame(const float CurrentWorldTime, const uint8 Action)
{
	if (CurrentWorldTime != WorldTime) return false;
	UConstituent::ErrorIfIdNotWithinRange(Action);
	if (NumActionsIncludingZero == 0)
	{
		ActionOne = Action;
	}
	else if (NumActionsIncludingZero == 1)
	{
		ActionTwo = Action;
	}
	else if (NumActionsIncludingZero == 2)
	{
		ActionThree = Action;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Action dropped because too many actions were executed in one frame."))
	}
	return true;
}

bool FActionSet::operator==(const FActionSet& Other) const
{
	if (WorldTime == Other.WorldTime && NumActionsIncludingZero == Other.NumActionsIncludingZero &&
		ActionZero == Other.ActionZero && ActionOne == Other.ActionOne && ActionTwo == Other.ActionTwo && ActionThree ==
		Other.ActionThree)
		return true;
	return false;
}

TArray<uint8> FActionSet::ToArray() const
{
	TArray<uint8> Array;
	Array.Add(ActionZero);
	if (NumActionsIncludingZero > 0)
	{
		Array.Add(ActionOne);
	}
	if (NumActionsIncludingZero > 1)
	{
		Array.Add(ActionTwo);
	}
	if (NumActionsIncludingZero > 2)
	{
		Array.Add(ActionThree);
	}
	return Array;
}

UConstituent::UConstituent()
{
	if (!HasAuthority())
	{
		bAwaitingClientInit = true;
	}
}

void UConstituent::Tick(float DeltaTime)
{
	//Stop incrementing if about to hit max value. Note that this will do so 655 seconds after state change.
	//-1 for rounding differences.
	if (TimeSinceLastActionSet < 65534 - DeltaTime * 100.0)
	{
		TimeSinceLastActionSet += DeltaTime * 100.0;
	}
}

void UConstituent::BeginDestroy()
{
	Super::BeginDestroy();
	if (!HasAuthority())
	{
		ClientDeinitialize();
	}
}

void UConstituent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(UConstituent, OwningSlotable, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UConstituent, LastActionSet, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UConstituent, TimeSinceLastActionSet, DefaultParams);
}

void UConstituent::OnRep_OwningSlotable()
{
	if (bAwaitingClientInit)
	{
		bAwaitingClientInit = false;
		ClientInitialize();
	}
}

void UConstituent::ClientInitialize()
{
}

void UConstituent::ServerInitialize()
{
	//Call events.
}

void UConstituent::ClientDeinitialize()
{
}

void UConstituent::ServerDeinitialize()
{
	OwningSlotable->OwningInventory->RemoveCardsOfOwner(InstanceId);
	//Call events.
}

void UConstituent::ExecuteAction(const uint8 ActionId, const bool bIsPredictableContext)
{
	if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy) UE_LOG(LogTemp, Error,
	                                                              TEXT("Tried to ExecuteAction as simulated proxy."));
	ErrorIfIdNotWithinRange(ActionId);
	if (HasAuthority())
	{
		const float ServerWorldTime = GetOwner()->GetWorld()->GetTimeSeconds();
		if (bIsPredictableContext)
		{
			//If the action is executed in a predictable context, we want to update PredictedLastActionSet so the predicted
			//client's version can get checked in FormCharacter if necessary.
			//We first try to add the action to the existing set if the set was created in this frame. If not we build a
			//new set.
			if (!PredictedLastActionSet.TryAddActionCheckIfSameFrame(ServerWorldTime, ActionId))
			{
				PredictedLastActionSet = FActionSet(ServerWorldTime, ActionId);
			}
			if (bEnableInputsAndPrediction && IsFormCharacter())
			{
				Predicted_OnExecute(ActionId, FSfPredictionFlags(false, false, false, true));
			}
		}
		//LastActionSet always needs to be updated if we execute an action, no matter the context.
		if (!LastActionSet.TryAddActionCheckIfSameFrame(ServerWorldTime, ActionId))
		{
			LastActionSet = FActionSet(ServerWorldTime, ActionId);
		}
		Server_OnExecute(ActionId);
		//We replicate this so that simulated proxies can know how long their previous action has ran for after they
		//become relevant.
		TimeSinceLastActionSet = 0;
		MARK_PROPERTY_DIRTY_FROM_NAME(UConstituent, LastActionSet, this);
		//We only mark this dirty when we execute.
		MARK_PROPERTY_DIRTY_FROM_NAME(UConstituent, TimeSinceLastActionSet, this);
	}
	else if (bEnableInputsAndPrediction && IsFormCharacter() && bIsPredictableContext && GetOwner()->GetLocalRole() ==
		ROLE_AutonomousProxy)
	{
		//On client we only want to execute if we are predicting.
		const float ClientWorldTime = GetOwner()->GetWorld()->GetTimeSeconds();
		//Same concept of checking whether to add or build the set like above.
		if (!PredictedLastActionSet.TryAddActionCheckIfSameFrame(ClientWorldTime, ActionId))
		{
			PredictedLastActionSet = FActionSet(ClientWorldTime, ActionId);
		}
		const UFormCharacterComponent* FormCharacter = GetFormCharacter();
		Predicted_OnExecute(ActionId, FSfPredictionFlags(FormCharacter->bClientStatesWereCorrected,
		                                           FormCharacter->bClientMetadataWasCorrected,
		                                           FormCharacter->bClientOnPlayback, false));
	}
	//Other roles change their states OnRep.
}

void UConstituent::ErrorIfIdNotWithinRange(const uint8 Id)
{
	checkf(Id < 64, TEXT("Action id not within range."));
}

int32 UConstituent::GetInstanceId()
{
	return InstanceId;
}

void UConstituent::OnRep_LastActionSet()
{
	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		for (const uint8 Id : LastActionSet.ToArray())
		{
			Autonomous_OnExecute(Id, static_cast<float>(TimeSinceLastActionSet) / 100.0);
		}
	}
	if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		for (const uint8 Id : LastActionSet.ToArray())
		{
			if (OwningSlotable->OwningInventory->OwningFormCore->IsFirstPerson())
			{
				SimulatedFP_OnExecute(Id, static_cast<float>(TimeSinceLastActionSet) / 100.0);
			}
			else
			{
				SimulatedTP_OnExecute(Id, static_cast<float>(TimeSinceLastActionSet) / 100.0);
			}
		}
	}
}
