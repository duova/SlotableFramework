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

FActionSet::FActionSet(const float InCurrentWorldTime, const uint8 InActionZero, const uint8 InActionOne,
                       const uint8 InActionTwo, const uint8 InActionThree): ActionZero(InActionZero),
                                                                            ActionOne(InActionOne),
                                                                            ActionTwo(InActionTwo),
                                                                            ActionThree(InActionThree),
                                                                            WorldTime(InCurrentWorldTime),
                                                                            bFlipToForceReplicate(0)
{
	if (InActionZero == 0)
	{
		UE_LOG(LogSfCore, Error, TEXT("Created ActionSet with no action zero."));
	}
	UConstituent::IsIdWithinRange(InActionZero);
	if (InActionOne != 0)
	{
		NumActionsIncludingZero = 1;
		UConstituent::IsIdWithinRange(InActionOne);
	}
	if (InActionTwo != 0)
	{
		NumActionsIncludingZero = 2;
		if (InActionOne == 0)
		{
			UE_LOG(LogSfCore, Error, TEXT("Created ActionSet with action two but no action one."));
		}
		UConstituent::IsIdWithinRange(InActionTwo);
	}
	if (InActionThree != 0)
	{
		NumActionsIncludingZero = 3;
		if (InActionTwo == 0)
		{
			UE_LOG(LogSfCore, Error, TEXT("Created ActionSet with action three but no action two."));
		}
		UConstituent::IsIdWithinRange(InActionThree);
	}
}

bool FActionSet::TryAddActionCheckIfSameFrame(const float InCurrentWorldTime, const uint8 InAction)
{
	if (InCurrentWorldTime != WorldTime) return false;
	UConstituent::IsIdWithinRange(InAction);
	if (NumActionsIncludingZero == 0)
	{
		ActionOne = InAction;
	}
	else if (NumActionsIncludingZero == 1)
	{
		ActionTwo = InAction;
	}
	else if (NumActionsIncludingZero == 2)
	{
		ActionThree = InAction;
	}
	else
	{
		UE_LOG(LogSfCore, Error, TEXT("Action dropped because too many actions were executed in one frame."))
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

bool FBufferedInput::operator==(const FBufferedInput& Other) const
{
	if (OwnedCardsRequired != Other.OwnedCardsRequired) return false;
	if (OwnedCardsRequiredGone != Other.OwnedCardsRequiredGone) return false;
	if (SharedCardsRequired != Other.SharedCardsRequired) return false;
	if (SharedCardsRequiredGone != Other.SharedCardsRequiredGone) return false;
	if (LifetimePredictedTimestamp != Other.LifetimePredictedTimestamp) return false;
	if (Delegate != Other.Delegate) return false;
	return true;
}

bool FBufferedInput::CheckConditionsMet(const UInventory* InInventoryToCheck,
                                        const UConstituent* InCurrentConstituent) const
{
	//This might look like a heavy check every time an input is pressed and when cards are added and removed. However,
	//note that most of the time only 1 for loop will run as it's likely that only 1 type of card will be checked.
	for (const TSubclassOf<UCardObject>& OwnedCardRequired : OwnedCardsRequired)
	{
		if (!OwnedCardRequired.Get()) continue;
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
		if (!OwnedCardRequiredGone.Get()) continue;
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
		if (!SharedCardRequired.Get()) continue;
		if (!InInventoryToCheck->Cards.FindByPredicate([SharedCardRequired](const FCard& Card)
		{
			return Card.OwnerConstituentInstanceId == 0 && Card.Class == SharedCardRequired;
		}))
		{
			return false;
		}
	}
	for (const TSubclassOf<UCardObject>& SharedCardRequiredGone : SharedCardsRequiredGone)
	{
		if (!SharedCardRequiredGone.Get()) continue;
		if (InInventoryToCheck->Cards.FindByPredicate([SharedCardRequiredGone](const FCard& Card)
		{
			return Card.OwnerConstituentInstanceId == 0 && Card.Class == SharedCardRequiredGone;
		}))
		{
			return false;
		}
	}
	return true;
}

uint32 GetTypeHash(const FBufferedInput& BufferedInput)
{
	return FCrc::MemCrc32(&BufferedInput, sizeof(FBufferedInput));
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
	DOREPLIFETIME_WITH_PARAMS_FAST(UConstituent, InstanceId, DefaultParams);
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
	if (!OwningSlotable)
	{
		UE_LOG(LogSfCore, Error, TEXT("Constituent of class %s has no OwningSlotable, destroying."), *GetClass()->GetName());
		Destroy();
	}
	else if (!OwningSlotable->OwningInventory)
	{
		UE_LOG(LogSfCore, Error, TEXT("Constituent of class %s has no OwningInventory, destroying."), *GetClass()->GetName());
		Destroy();
	}
	else
	{
		FormCore = OwningSlotable->OwningInventory->OwningFormCore;
		UE_LOG(LogSfCore, Error, TEXT("Constituent of class %s has no FormCoreComponent, destroying."), *GetClass()->GetName());
		Destroy();
	}
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

void UConstituent::ExecuteAction(const int32 InActionId, const bool bInIsPredictableContext)
{
	if (InActionId < 0 || InActionId > 63)
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("ExecuteAction called with an out of range ActionId of %i on UConstituent class %s"), InActionId,
		       *GetClass()->GetName());
		return;
	}
	InternalExecuteAction(InActionId, bInIsPredictableContext);
}

void UConstituent::InternalExecuteAction(const uint8 InActionId, const bool bInIsPredictableContext)
{
	if (!GetOwner()) return;
	if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Tried to ExecuteAction as simulated proxy on UConstituent class %s."), *GetClass()->GetName());
		return;
	}
	if (!IsIdWithinRange(InActionId)) return;
	if (HasAuthority())
	{
		const float ServerWorldTime = GetOwner()->GetWorld()->GetTimeSeconds();
		if (bInIsPredictableContext)
		{
			//If the action is executed in a predictable context, we want to update PredictedLastActionSet so the predicted
			//client's version can get checked in FormCharacter if necessary.
			//We first try to add the action to the existing set if the set was created in this frame. If not we build a
			//new set.
			if (!PredictedLastActionSet.TryAddActionCheckIfSameFrame(ServerWorldTime, InActionId))
			{
				PredictedLastActionSet = FActionSet(ServerWorldTime, InActionId);
			}
			bPredictedLastActionSetUpdated = true;
			TimeSincePredictedLastActionSet.SetFloat(0);
			if (bEnableInputsAndPrediction && IsFormCharacter())
			{
				Predicted_OnExecute(InActionId, GetFormCharacter()->IsReplaying(), FormCore->IsFirstPerson(), true);
			}
		}
		//LastActionSet always needs to be updated if we execute an action, no matter the context.
		if (!LastActionSet.TryAddActionCheckIfSameFrame(ServerWorldTime, InActionId))
		{
			const bool bFlipToReplicate = LastActionSet.bFlipToForceReplicate;
			LastActionSet = FActionSet(ServerWorldTime, InActionId);
			//Make a guaranteed change to make sure OnRep is fired.
			LastActionSet.bFlipToForceReplicate = !bFlipToReplicate;
		}
		Server_OnExecute(InActionId);
		//We replicate this so that simulated proxies can know how long their previous action has ran for after they
		//become relevant.
		LastActionSetTimestamp = OwningSlotable->OwningInventory->OwningFormCore->GetNonCompensatedServerWorldTime();
		MARK_PROPERTY_DIRTY_FROM_NAME(UConstituent, LastActionSet, this);
		//We only mark this dirty when we execute.
		MARK_PROPERTY_DIRTY_FROM_NAME(UConstituent, LastActionSetTimestamp, this);
	}
	else if (bEnableInputsAndPrediction && IsFormCharacter() && bInIsPredictableContext && GetOwner()->
		GetLocalRole() ==
		ROLE_AutonomousProxy)
	{
		//On client we only want to execute if we are predicting.
		const float ClientWorldTime = GetOwner()->GetWorld()->GetTimeSeconds();
		//Same concept of checking whether to add or build the set like above.
		if (!PredictedLastActionSet.TryAddActionCheckIfSameFrame(ClientWorldTime, InActionId))
		{
			PredictedLastActionSet = FActionSet(ClientWorldTime, InActionId);
		}
		bPredictedLastActionSetUpdated = true;
		TimeSincePredictedLastActionSet.SetFloat(0);
		Predicted_OnExecute(InActionId, GetFormCharacter()->IsReplaying(), FormCore->IsFirstPerson(), false);
	}
	//Other roles change their states OnRep.
}

bool UConstituent::IsIdWithinRange(const uint8 InId)
{
	//ActionId must be under 64 due to the 6-bit serialization.
	if (InId > 63)
	{
		UE_LOG(LogSfCore, Error, TEXT("Action id not within range."));
		return false;
	}
	return true;
}

int32 UConstituent::GetInstanceId() const
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
			Autonomous_OnExecute(Id, TimeSinceExecution, FormCore->IsFirstPerson());
		}
	}
	if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		for (const uint8 Id : LastActionSet.ToArray())
		{
			Simulated_OnExecute(Id, TimeSinceExecution, FormCore->IsFirstPerson());
		}
	}
}

void UConstituent::IncrementTimeSincePredictedLastActionSet(const float InTimePassed)
{
	TimeSincePredictedLastActionSet.SetFloat(TimeSincePredictedLastActionSet.GetFloat() + InTimePassed);
}

USfQuery* UConstituent::GetQuery(const TSubclassOf<USfQuery> QueryClass) const
{
	if (!FormCore) return nullptr;
	if (!FormCore->GetFormQuery())
	{
		UE_LOG(LogSfCore, Error, TEXT("GetQuery called on a UConstituent of class %s without a FormQueryComponent."),
		       *GetClass()->GetName());
		return nullptr;
	}
	if (!QueryClass.Get())
	{
		UE_LOG(LogSfCore, Error, TEXT("GetQuery called on a UConstituent of class %s with an empty TSubclassOf."),
		       *GetClass()->GetName());
		return nullptr;
	}
	for (const TPair<USfQuery*, uint16>& Pair : FormCore->GetFormQuery()->ActiveQueryDependentCountPair)
	{
		if (Pair.Key->GetClass() == QueryClass.Get())
		{
			return Pair.Key;
		}
	}
	UE_LOG(LogTemp, Error,
	       TEXT(
		       "GetQuery could not find query of class %s for UConstituent %s. Is the query a dependency of the constituent?"
	       ), *QueryClass->GetName(), *GetClass()->GetName());
	return nullptr;
}

void UConstituent::BufferInput(const float InTimeout,
                               const FBufferedInputDelegate& EventToBind,
                               const TArray<TSubclassOf<UCardObject>>& InOwnedCardsRequiredToActivate,
                               const TArray<TSubclassOf<UCardObject>>& InOwnedCardsRequiredGoneToActivate,
                               const TArray<TSubclassOf<UCardObject>>& InSharedCardsRequiredToActivate,
                               const TArray<TSubclassOf<UCardObject>>& InSharedCardsRequiredGoneToActivate)
{
	InternalBufferInput(InTimeout, EventToBind, InOwnedCardsRequiredToActivate, InOwnedCardsRequiredGoneToActivate,
	                    InSharedCardsRequiredToActivate, InSharedCardsRequiredGoneToActivate);
}

void UConstituent::InternalBufferInput(const float InTimeout,
                                       const FBufferedInputDelegate& EventToBind,
                                       const TArray<TSubclassOf<UCardObject>>& InOwnedCardsRequiredToActivate,
                                       const TArray<TSubclassOf<UCardObject>>& InOwnedCardsRequiredGoneToActivate,
                                       const TArray<TSubclassOf<UCardObject>>& InSharedCardsRequiredToActivate,
                                       const TArray<TSubclassOf<UCardObject>>& InSharedCardsRequiredGoneToActivate)
{
	if (InTimeout < 0)
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Timeout on BufferedInput is negative on UConstituent class %s."), *GetClass()->GetName());
	}
	if (InTimeout > 1.5)
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("Buffered inputs can only have a timeout of less than 1.5 seconds. Called in UConstituent class %s."
		       ), *GetClass()->GetName());
	}
	BufferedInputs.Add(FBufferedInput(InOwnedCardsRequiredToActivate, InOwnedCardsRequiredGoneToActivate,
	                                  InSharedCardsRequiredToActivate, InSharedCardsRequiredGoneToActivate,
	                                  GetFormCharacter()->CalculateFuturePredictedTimestamp(InTimeout), EventToBind));
	//We check buffed inputs instantly just in case they can be run already.
	OwningSlotable->OwningInventory->UpdateAndRunBufferedInputs(this);
}

void UConstituent::HandleBufferInputTimeout()
{
	for (auto It = BufferedInputs.CreateIterator(); It; ++It)
	{
		if (GetFormCharacter()->CalculateTimeUntilPredictedTimestamp(It->LifetimePredictedTimestamp) < 0)
		{
			It.RemoveCurrent();
		}
	}
}

UFormCoreComponent* UConstituent::GetFormCoreComponent() const
{
	if (!FormCore)
	{
		UE_LOG(LogSfCore, Error, TEXT("UConstituent of class %s does not have FormCoreComponent."), *GetClass()->GetName())
	}
	return FormCore;
}
