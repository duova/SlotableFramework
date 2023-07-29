// Fill out your copyright notice in the Description page of Project Settings.


#include "Constituent.h"

#include "CardObject.h"
#include "FormCharacterComponent.h"
#include "FormCoreComponent.h"
#include "Inventory.h"
#include "Slotable.h"
#include "FormQueryComponent.h"
#include "Net/UnrealNetwork.h"

FActionSet::FActionSet(): NumActionsIncludingZero(0), ActionZero(0), ActionOne(0), ActionTwo(0), ActionThree(0),
                          WorldTime(0),
                          bFlipToForceReplicate(0)
{
}

FActionSet::FActionSet(const float CurrentWorldTime, const uint8 ActionZero, const uint8 ActionOne,
                       const uint8 ActionTwo, const uint8 ActionThree): ActionZero(ActionZero), ActionOne(ActionOne),
                                                                        ActionTwo(ActionTwo),
                                                                        ActionThree(ActionThree),
                                                                        WorldTime(CurrentWorldTime),
                                                                        bFlipToForceReplicate(0)
{
	if (ActionZero == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Created ActionSet with no action zero."));
	}
	UConstituent::IsIdWithinRange(ActionZero);
	if (ActionOne != 0)
	{
		NumActionsIncludingZero = 1;
		UConstituent::IsIdWithinRange(ActionOne);
	}
	if (ActionTwo != 0)
	{
		NumActionsIncludingZero = 2;
		if (ActionOne == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Created ActionSet with action two but no action one."));
		}
		UConstituent::IsIdWithinRange(ActionTwo);
	}
	if (ActionThree != 0)
	{
		NumActionsIncludingZero = 3;
		if (ActionTwo == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Created ActionSet with action three but no action two."));
		}
		UConstituent::IsIdWithinRange(ActionThree);
	}
}

bool FActionSet::TryAddActionCheckIfSameFrame(const float CurrentWorldTime, const uint8 Action)
{
	if (CurrentWorldTime != WorldTime) return false;
	UConstituent::IsIdWithinRange(Action);
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
		Other.ActionThree && bFlipToForceReplicate == Other.bFlipToForceReplicate)
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

FBufferedInput::FBufferedInput(): LifetimePredictedTimestamp(0)
{
}

FBufferedInput::FBufferedInput(const TArray<TSubclassOf<UCardObject>>& InOwnedCardsRequired,
                               const TArray<TSubclassOf<UCardObject>>& InOwnedCardsRequiredGone,
                               const TArray<TSubclassOf<UCardObject>>& InSharedCardsRequired,
                               const TArray<TSubclassOf<UCardObject>>& InSharedCardsRequiredGone,
                               const float InLifetimePredictedTimestamp,
                               const FBufferedInputDelegate& InDelegate)
{
	OwnedCardsRequired = InOwnedCardsRequired;
	OwnedCardsRequiredGone = InOwnedCardsRequiredGone;
	SharedCardsRequired = InSharedCardsRequired;
	SharedCardsRequiredGone = InSharedCardsRequiredGone;
	LifetimePredictedTimestamp = InLifetimePredictedTimestamp;
	Delegate = InDelegate;
}

bool FBufferedInput::CheckConditionsMet(const UInventory* InInventoryToCheck, const UConstituent* InCurrentConstituent)
{
	//This might look like a heavy check every time an input is pressed and when cards are added and removed. However,
	//note that most of the time only 1 for loop will run as it's likely that only 1 type of card will be checked.
	for (const TSubclassOf<UCardObject>& OwnedCardRequired : OwnedCardsRequired)
	{
		if (!InInventoryToCheck->Cards.FindByPredicate([OwnedCardRequired, InCurrentConstituent](const FCard& Card)
		{
			return Card.OwnerConstituentInstanceId == InCurrentConstituent->InstanceId && Card.Class ==
				OwnedCardRequired;
		}))
		{
			return false;
		}
	}
	for (const TSubclassOf<UCardObject>& OwnedCardRequiredGone : OwnedCardsRequiredGone)
	{
		if (InInventoryToCheck->Cards.FindByPredicate([OwnedCardRequiredGone, InCurrentConstituent](const FCard& Card)
		{
			return Card.OwnerConstituentInstanceId == InCurrentConstituent->InstanceId && Card.Class ==
				OwnedCardRequiredGone;
		}))
		{
			return false;
		}
	}
	for (const TSubclassOf<UCardObject>& SharedCardRequired : SharedCardsRequired)
	{
		if (!InInventoryToCheck->Cards.FindByPredicate([SharedCardRequired, InCurrentConstituent](const FCard& Card)
		{
			return Card.OwnerConstituentInstanceId == 0 && Card.Class == SharedCardRequired;
		}))
		{
			return false;
		}
	}
	for (const TSubclassOf<UCardObject>& SharedCardRequiredGone : SharedCardsRequiredGone)
	{
		if (InInventoryToCheck->Cards.FindByPredicate([SharedCardRequiredGone, InCurrentConstituent](const FCard& Card)
		{
			return Card.OwnerConstituentInstanceId == 0 && Card.Class == SharedCardRequiredGone;
		}))
		{
			return false;
		}
	}
	return true;
}

UConstituent::UConstituent()
{
}

void UConstituent::BeginDestroy()
{
	Super::BeginDestroy();
	if (!GetOwner()) return;
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
	DOREPLIFETIME_WITH_PARAMS_FAST(UConstituent, LastActionSetTimestamp, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UConstituent, OriginatingConstituent, DefaultParams);
}

void UConstituent::OnRep_OwningSlotable()
{
	if (!GetOwner()) return;
	if (bAwaitingClientInit && !HasAuthority())
	{
		bAwaitingClientInit = false;
		ClientInitialize();
	}
}

void UConstituent::SetFormCore()
{
	checkf(OwningSlotable, TEXT("Constituent has no OwningSlotable."));
	checkf(OwningSlotable->OwningInventory, TEXT("Constituent has no OwningInventory."));
	FormCore = OwningSlotable->OwningInventory->OwningFormCore;
	checkf(FormCore, TEXT("Constituent has no FormCore."));
}

void UConstituent::ClientInitialize()
{
	SetFormCore();
	FormCore->ConstituentRegistry.Add(this);
	Client_Initialize();
}

void UConstituent::ServerInitialize()
{
	SetFormCore();
	FormCore->ConstituentRegistry.Add(this);
	FormCore->GetFormQuery()->RegisterQueryDependencies(QueryDependencyClasses);
	Server_Initialize();
}

void UConstituent::ClientDeinitialize()
{
	Client_Deinitialize();
	FormCore->ConstituentRegistry.Remove(this);
}

void UConstituent::ServerDeinitialize()
{
	Server_Deinitialize();
	FormCore->GetFormQuery()->UnregisterQueryDependencies(QueryDependencyClasses);
	FormCore->ConstituentRegistry.Remove(this);
	OwningSlotable->OwningInventory->RemoveCardsOfOwner(InstanceId);
}

void UConstituent::ExecuteAction(const uint8 ActionId, const bool bIsPredictableContext)
{
	if (!GetOwner()) return;
	if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		UE_LOG(LogTemp, Error,
		       TEXT("Tried to ExecuteAction as simulated proxy."));
		return;
	}
	if (!IsIdWithinRange(ActionId)) return;
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
			bPredictedLastActionSetUpdated = true;
			TimeSincePredictedLastActionSet.SetFloat(0);
			if (bEnableInputsAndPrediction && IsFormCharacter())
			{
				Predicted_OnExecute(ActionId, GetFormCharacter()->IsReplaying());
			}
		}
		//LastActionSet always needs to be updated if we execute an action, no matter the context.
		if (!LastActionSet.TryAddActionCheckIfSameFrame(ServerWorldTime, ActionId))
		{
			const bool bFlipToReplicate = LastActionSet.bFlipToForceReplicate;
			LastActionSet = FActionSet(ServerWorldTime, ActionId);
			//Make a guaranteed change to make sure OnRep is fired.
			LastActionSet.bFlipToForceReplicate = !bFlipToReplicate;
		}
		Server_OnExecute(ActionId);
		//We replicate this so that simulated proxies can know how long their previous action has ran for after they
		//become relevant.
		LastActionSetTimestamp = OwningSlotable->OwningInventory->OwningFormCore->GetNonCompensatedServerWorldTime();
		MARK_PROPERTY_DIRTY_FROM_NAME(UConstituent, LastActionSet, this);
		//We only mark this dirty when we execute.
		MARK_PROPERTY_DIRTY_FROM_NAME(UConstituent, LastActionSetTimestamp, this);
	}
	else if (bEnableInputsAndPrediction && IsFormCharacter() && bIsPredictableContext && GetOwner()->
		GetLocalRole() ==
		ROLE_AutonomousProxy)
	{
		//On client we only want to execute if we are predicting.
		const float ClientWorldTime = GetOwner()->GetWorld()->GetTimeSeconds();
		//Same concept of checking whether to add or build the set like above.
		if (!PredictedLastActionSet.TryAddActionCheckIfSameFrame(ClientWorldTime, ActionId))
		{
			PredictedLastActionSet = FActionSet(ClientWorldTime, ActionId);
		}
		bPredictedLastActionSetUpdated = true;
		TimeSincePredictedLastActionSet.SetFloat(0);
		Predicted_OnExecute(ActionId, GetFormCharacter()->IsReplaying());
	}
	//Other roles change their states OnRep.
}

bool UConstituent::IsIdWithinRange(const uint8 Id)
{
	if (Id >= 64)
	{
		UE_LOG(LogTemp, Error, TEXT("Action id not within range."));
		return false;
	}
	return true;
}

uint8 UConstituent::GetInstanceId() const
{
	return InstanceId;
}

UConstituent* UConstituent::GetTrueOriginatingConstituent() const
{
	//We search down the constituent origin chain and return the last constituent.
	UConstituent* CurrentConstituent = const_cast<UConstituent*>(this);
	UConstituent* CheckedOriginatingConstituent = OriginatingConstituent;
	while (true)
	{
		if (CheckedOriginatingConstituent == nullptr)
		{
			return CurrentConstituent;
		}
		CurrentConstituent = CheckedOriginatingConstituent;
		CheckedOriginatingConstituent = CheckedOriginatingConstituent->OriginatingConstituent;
	}
}

UConstituent* UConstituent::GetOriginatingConstituent() const
{
	return OriginatingConstituent;
}

void UConstituent::OnRep_LastActionSet()
{
	if (!FormCore) return;
	const float TimeSinceExecution = FormCore->CalculateTimeSinceServerTimestamp(LastActionSetTimestamp);
	if (!GetOwner()) return;
	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		for (const uint8 Id : LastActionSet.ToArray())
		{
			Autonomous_OnExecute(Id, TimeSinceExecution);
		}
	}
	if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		for (const uint8 Id : LastActionSet.ToArray())
		{
			if (OwningSlotable->OwningInventory->OwningFormCore->IsFirstPerson())
			{
				SimulatedFP_OnExecute(Id, TimeSinceExecution);
			}
			else
			{
				SimulatedTP_OnExecute(Id, TimeSinceExecution);
			}
		}
	}
}

void UConstituent::IncrementTimeSincePredictedLastActionSet(const float Time)
{
	TimeSincePredictedLastActionSet.SetFloat(TimeSincePredictedLastActionSet.GetFloat() + Time);
}

USfQuery* UConstituent::GetQuery(const TSubclassOf<USfQuery> QueryClass) const
{
	if (!FormCore) return nullptr;
	if (!FormCore->GetFormQuery())
	{
		UE_LOG(LogTemp, Error, TEXT("GetQuery called on a constituent without a FormQueryComponent"));
	}
	for (const TPair<USfQuery*, uint16>& Pair : FormCore->GetFormQuery()->ActiveQueryDependentCountPair)
	{
		if (Pair.Key->GetClass() == QueryClass.Get())
		{
			return Pair.Key;
		}
	}
	UE_LOG(LogTemp, Error, TEXT("GetQuery could not find query. Is the query a dependency of the constituent?"));
	return nullptr;
}

void UConstituent::BufferInput(const TArray<TSubclassOf<UCardObject>> InOwnedCardsRequiredToActivate,
                               const TArray<TSubclassOf<UCardObject>> InOwnedCardsRequiredGoneToActivate,
                               const TArray<TSubclassOf<UCardObject>> InSharedCardsRequiredToActivate,
                               const TArray<TSubclassOf<UCardObject>> InSharedCardsRequiredGoneToActivate,
                               const float Timeout,
                               const FBufferedInputDelegate& EventToBind)
{
	const uint16 Index = BufferedInputs.Emplace(InOwnedCardsRequiredToActivate, InOwnedCardsRequiredGoneToActivate,
	                       InSharedCardsRequiredToActivate, InSharedCardsRequiredGoneToActivate,
	                       GetFormCharacter()->CalculateFuturePredictedTimestamp(Timeout), EventToBind);
	//We check buffed inputs instantly just in case they can be run already.
	OwningSlotable->OwningInventory->UpdateAndRunBufferedInputs(this);
}

void UConstituent::HandleBufferInputTimeout()
{
	TArray<FBufferedInput*> ToRemove;
	for (FBufferedInput& BufferedInput : BufferedInputs)
	{
		if (GetFormCharacter()->CalculateTimeUntilPredictedTimestamp(BufferedInput.LifetimePredictedTimestamp) < 0)
		{
			ToRemove.Add(&BufferedInput);
		}
	}
	for (const FBufferedInput* Element : ToRemove)
	{
		BufferedInputs.Remove(*Element);
	}
}
