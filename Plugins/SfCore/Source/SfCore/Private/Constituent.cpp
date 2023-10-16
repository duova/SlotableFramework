// Fill out your copyright notice in the Description page of Project Settings.


#include "Constituent.h"

#include "CardObject.h"
#include "FormCharacterComponent.h"
#include "FormCoreComponent.h"
#include "Inventory.h"
#include "Slotable.h"
#include "FormQueryComponent.h"
#include "Net/UnrealNetwork.h"

FActionSet::FActionSet(): NumActionsMinusOne(0), ActionZero(0), ActionOne(0), ActionTwo(0), ActionThree(0),
                          WorldTime(0)
{
}

FActionSet::FActionSet(const float InCurrentWorldTime, const uint8 InActionZero, const uint8 InActionOne,
                       const uint8 InActionTwo, const uint8 InActionThree): NumActionsMinusOne(0),
                                                                            ActionZero(InActionZero),
                                                                            ActionOne(InActionOne),
                                                                            ActionTwo(InActionTwo),
                                                                            ActionThree(InActionThree),
                                                                            WorldTime(InCurrentWorldTime)
{
	if (InActionZero == 0)
	{
		UE_LOG(LogSfCore, Error, TEXT("Created ActionSet with no action zero."));
	}
	UConstituent::IsIdWithinRange(InActionZero);
	if (InActionOne != 0)
	{
		NumActionsMinusOne = 1;
		UConstituent::IsIdWithinRange(InActionOne);
	}
	if (InActionTwo != 0)
	{
		NumActionsMinusOne = 2;
		if (InActionOne == 0)
		{
			UE_LOG(LogSfCore, Error, TEXT("Created ActionSet with action two but no action one."));
		}
		UConstituent::IsIdWithinRange(InActionTwo);
	}
	if (InActionThree != 0)
	{
		NumActionsMinusOne = 3;
		if (InActionTwo == 0)
		{
			UE_LOG(LogSfCore, Error, TEXT("Created ActionSet with action three but no action two."));
		}
		UConstituent::IsIdWithinRange(InActionThree);
	}
}

FActionSet::FActionSet(const float InCurrentWorldTime, const uint8 InActionZero, const FBitWriter& InActionZeroParams,
	const uint8 InActionOne, const FBitWriter& InActionOneParams, const uint8 InActionTwo, const FBitWriter& InActionTwoParams,
	const uint8 InActionThree, const FBitWriter& InActionThreeParams): NumActionsMinusOne(0),
																			ActionZero(InActionZero),
																			ActionOne(InActionOne),
																			ActionTwo(InActionTwo),
																			ActionThree(InActionThree),
																			WorldTime(InCurrentWorldTime)
{
	if (InActionZero == 0)
	{
		UE_LOG(LogSfCore, Error, TEXT("Created ActionSet with no action zero."));
	}
	UConstituent::IsIdWithinRange(InActionZero);
	if (InActionOne != 0)
	{
		NumActionsMinusOne = 1;
		UConstituent::IsIdWithinRange(InActionOne);
	}
	if (InActionTwo != 0)
	{
		NumActionsMinusOne = 2;
		if (InActionOne == 0)
		{
			UE_LOG(LogSfCore, Error, TEXT("Created ActionSet with action two but no action one."));
		}
		UConstituent::IsIdWithinRange(InActionTwo);
	}
	if (InActionThree != 0)
	{
		NumActionsMinusOne = 3;
		if (InActionTwo == 0)
		{
			UE_LOG(LogSfCore, Error, TEXT("Created ActionSet with action three but no action two."));
		}
		UConstituent::IsIdWithinRange(InActionThree);
	}
	ActionZeroParams = BitWriterToBitArray(InActionZeroParams);
	ActionOneParams = BitWriterToBitArray(InActionOneParams);
	ActionTwoParams = BitWriterToBitArray(InActionTwoParams);
	ActionThreeParams = BitWriterToBitArray(InActionThreeParams);
}

bool FActionSet::TryAddActionCheckIfSameFrame(const float InCurrentWorldTime, const uint8 InAction, const FBitWriter& InParams = FBitWriter())
{
	if (InCurrentWorldTime != WorldTime) return false;
	UConstituent::IsIdWithinRange(InAction);
	if (NumActionsMinusOne == 0)
	{
		ActionOne = InAction;
		ActionOneParams = BitWriterToBitArray(InParams);
		NumActionsMinusOne++;
	}
	else if (NumActionsMinusOne == 1)
	{
		ActionTwo = InAction;
		ActionTwoParams = BitWriterToBitArray(InParams);
		NumActionsMinusOne++;
	}
	else if (NumActionsMinusOne == 2)
	{
		ActionThree = InAction;
		ActionThreeParams = BitWriterToBitArray(InParams);
		NumActionsMinusOne++;
	}
	else
	{
		UE_LOG(LogSfCore, Error, TEXT("Action dropped because too many actions were executed in one frame."))
	}
	return true;
}

bool FActionSet::operator==(const FActionSet& Other) const
{
	if (NumActionsMinusOne == Other.NumActionsMinusOne &&
		ActionZero == Other.ActionZero && ActionOne == Other.ActionOne && ActionTwo == Other.ActionTwo && ActionThree ==
		Other.ActionThree)
		return true;
	return false;
}

bool FActionSet::operator!=(const FActionSet& Other) const
{
	return !(*this == Other);
}

TMap<uint8, TBitArray<>> FActionSet::ToMap() const
{
	TMap<uint8, TBitArray<>> Map;
	Map.Add(ActionZero, ActionZeroParams);
	if (NumActionsMinusOne > 0)
	{
		Map.Add(ActionOne, ActionOneParams);
	}
	if (NumActionsMinusOne > 1)
	{
		Map.Add(ActionTwo, ActionTwoParams);
	}
	if (NumActionsMinusOne > 2)
	{
		Map.Add(ActionThree, ActionThreeParams);
	}
	return Map;
}

bool FActionSet::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << *this;
	return true;
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

bool FBufferedInput::CheckConditionsMet(const UConstituent* InCurrentConstituent) const
{
	UInventory* InventoryToCheck = InCurrentConstituent->OwningSlotable->OwningInventory;
	//This might look like a heavy check every time an input is pressed and when cards are added and removed. However,
	//note that most of the time only 1 for loop will run as it's likely that only 1 type of card will be checked.
	for (const TSubclassOf<UCardObject>& OwnedCardRequired : OwnedCardsRequired)
	{
		if (!OwnedCardRequired.Get()) continue;
		if (!InventoryToCheck->Cards.FindByPredicate([OwnedCardRequired, InCurrentConstituent](const FCard& Card)
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
		if (InventoryToCheck->Cards.FindByPredicate([OwnedCardRequiredGone, InCurrentConstituent](const FCard& Card)
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
		if (!InventoryToCheck->Cards.FindByPredicate([SharedCardRequired](const FCard& Card)
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
		if (InventoryToCheck->Cards.FindByPredicate([SharedCardRequiredGone](const FCard& Card)
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
	UBlueprintGeneratedClass::BindDynamicDelegates(GetClass(), this);
	CurrentBitReader = FBitReader();
}

void UConstituent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(UConstituent, OwningSlotable, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UConstituent, OriginatingConstituent, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UConstituent, InstanceId, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UConstituent, FormCore, DefaultParams);
}

void UConstituent::SetFormCore()
{
	if (!OwningSlotable)
	{
		UE_LOG(LogSfCore, Error, TEXT("Constituent of class %s has no OwningSlotable, destroying."),
		       *GetClass()->GetName());
		Destroy();
	}
	else if (!OwningSlotable->OwningInventory)
	{
		UE_LOG(LogSfCore, Error, TEXT("Constituent of class %s has no OwningInventory, destroying."),
		       *GetClass()->GetName());
		Destroy();
	}
	else if (!OwningSlotable->OwningInventory->OwningFormCore)
	{
		UE_LOG(LogSfCore, Error, TEXT("Constituent of class %s has no FormCoreComponent, destroying."),
		       *GetClass()->GetName());
		Destroy();
	}
	FormCore = OwningSlotable->OwningInventory->OwningFormCore;
	MARK_PROPERTY_DIRTY_FROM_NAME(UConstituent, FormCore, this);
}

void UConstituent::ServerInitialize()
{
	SetFormCore();
	FormCore->ConstituentRegistry.Add(this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, ConstituentRegistry, FormCore);
	if (QueryDependencyClasses.Num() != 0)
	{
		if (FormCore->GetFormQuery())
		{
			FormCore->GetFormQuery()->RegisterQueryDependencies(QueryDependencyClasses);
		}
		else if (!bQueryDependenciesAreOptional)
		{
			UE_LOG(LogSfCore, Error,
			       TEXT(
				       "UConstituent class %s has query dependencies but was inserted into a form without a UFormQueryComponent. Set bQueryDependenciesAreOptional to true if this is intentional."
			       ), *GetClass()->GetName());
		}
	}
	Server_Initialize();
}

void UConstituent::ServerDeinitialize()
{
	Server_Deinitialize();
	if (FormCore->GetFormQuery())
	{
		FormCore->GetFormQuery()->UnregisterQueryDependencies(QueryDependencyClasses);
	}
	FormCore->ConstituentRegistry.Remove(this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormCoreComponent, ConstituentRegistry, FormCore);
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
	InternalExecuteAction(InActionId, bInIsPredictableContext, FBitWriter());
}

void UConstituent::ExecuteActionWithParams(UConstituent* Target, const int32 InActionId, const bool bInIsPredictableContext,
                                           UStruct* InActionParams)
{
	UE_LOG(LogSfCore, Error,
	       TEXT(
		       "Normal implementation of ExecuteActionWithParams should not be called. Check if the thunk is implemented properly."
	       ));
}

void UConstituent::InternalExecuteAction(const uint8 InActionId, const bool bInIsPredictableContext, const FBitWriter& SerializedParams)
{
	if (!GetOwner()) return;
	const UFormCharacterComponent* FormCharacter = FormCore->FormCharacter;
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
			if (!PredictedLastActionSet.TryAddActionCheckIfSameFrame(ServerWorldTime, InActionId, SerializedParams))
			{
				PredictedLastActionSet = FActionSet(ServerWorldTime, InActionId, SerializedParams);
			}
			TimeSincePredictedLastActionSet.SetFloat(0);
			if (bEnableInputsAndPrediction && FormCharacter)
			{
				if (Predicted_OnExecute.IsBound())
				{
					InternalPredictedOnExecute(InActionId, FormCharacter->IsReplaying(), FormCore->IsFirstPerson(),
					                           true, BitWriterToBitArray(SerializedParams));
				}
			}
		}
		//LastActionSet always needs to be updated if we execute an action, no matter the context.
		if (!LastActionSet.TryAddActionCheckIfSameFrame(ServerWorldTime, InActionId, SerializedParams))
		{
			LastActionSet = FActionSet(ServerWorldTime, InActionId, SerializedParams);
		}
		if (Server_OnExecute.IsBound())
		{
			InternalServerOnExecute(InActionId, BitWriterToBitArray(SerializedParams));
		}
		LastActionSetTimestamp = OwningSlotable->OwningInventory->OwningFormCore->GetNonCompensatedServerFormTime();
		//Flag so FormCoreComponent can call NetMulticastClientPerformActionSet on the LastActionSet.
		bLastActionSetPendingClientExecution = true;
	}
	else if (bEnableInputsAndPrediction && FormCharacter && bInIsPredictableContext && GetOwner()->
		GetLocalRole() ==
		ROLE_AutonomousProxy)
	{
		//On client we only want to execute if we are predicting.
		const float ClientWorldTime = GetOwner()->GetWorld()->GetTimeSeconds();
		//Same concept of checking whether to add or build the set like above.
		if (!PredictedLastActionSet.TryAddActionCheckIfSameFrame(ClientWorldTime, InActionId, SerializedParams))
		{
			PredictedLastActionSet = FActionSet(ClientWorldTime, InActionId, SerializedParams);
		}
		TimeSincePredictedLastActionSet.SetFloat(0);
		if (Predicted_OnExecute.IsBound())
		{
			InternalPredictedOnExecute(InActionId, FormCharacter->IsReplaying(), FormCore->IsFirstPerson(), false, BitWriterToBitArray(SerializedParams));
		}
	}
}

void UConstituent::SerializeGenericStruct(FProperty* Property, void* StructPtr, FArchive& Ar)
{
	if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		// Walk the structs' properties.
		for (TFieldIterator<FProperty> PropertyIt(StructProperty->Struct); PropertyIt; ++PropertyIt)
		{
			for (int32 ArrayIndex = 0; ArrayIndex < PropertyIt->ArrayDim; ArrayIndex++)
			{
				void* ValuePtr = PropertyIt->ContainerPtrToValuePtr<void>(StructPtr, ArrayIndex);
				SerializeProperty(*PropertyIt, ValuePtr, Ar);
				if (Ar.IsError())
				{
					UE_LOG(LogSfCore, Error, TEXT("Error serializing property in generic struct."));
				}
			}
		}
	}
}

void UConstituent::SerializeProperty(FProperty* Property, void* ValuePtr, FArchive& Ar)
{
	const bool bIsSaving = Ar.IsSaving();
	
	if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
	{
		if (NumericProperty->IsFloatingPoint())
		{
			float FloatValue = bIsSaving ? NumericProperty->GetFloatingPointPropertyValue(ValuePtr) : 0;
			Ar << FloatValue;
			if (!bIsSaving)
			{
				NumericProperty->SetFloatingPointPropertyValue(ValuePtr, FloatValue);
			}
		}
		else if (NumericProperty->IsInteger())
		{
			int64 IntValue = bIsSaving ? NumericProperty->GetSignedIntPropertyValue(ValuePtr) : 0;
			Ar << IntValue;
			if (!bIsSaving)
			{
				NumericProperty->SetIntPropertyValue(ValuePtr, IntValue);
			}
		}
	}
	
	else if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
	{
		bool bBoolValue = bIsSaving ? BoolProperty->GetPropertyValue(ValuePtr) : false;
		Ar.SerializeBits(&bBoolValue, 1);
		if (!bIsSaving)
		{
			BoolProperty->SetPropertyValue(ValuePtr, bBoolValue);
		}
	}
	
	else if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
	{
		FName NameValue = bIsSaving ? NameProperty->GetPropertyValue(ValuePtr) : FName();
		Ar << NameValue;
		if (!bIsSaving)
		{
			NameProperty->SetPropertyValue(ValuePtr, NameValue);
		}
	}
	
	else if (const FStrProperty* StringProperty = CastField<FStrProperty>(Property))
	{
		FString StringValue = bIsSaving ? StringProperty->GetPropertyValue(ValuePtr) : FString();
		Ar << StringValue;
		if (!bIsSaving)
		{
			StringProperty->SetPropertyValue(ValuePtr, StringValue);
		}
	}
	
	else if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
	{
		FText TextValue = bIsSaving ? TextProperty->GetPropertyValue(ValuePtr) : FText();
		Ar << TextValue;
		if (!bIsSaving)
		{
			TextProperty->SetPropertyValue(ValuePtr, TextValue);
		}
	}
	
	else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
	{
		FScriptArrayHelper Helper(ArrayProperty, ValuePtr);
		for (int32 i = 0, n = Helper.Num(); i < n; ++i)
		{
			SerializeProperty(ArrayProperty->Inner, Helper.GetRawPtr(i), Ar);
		}
	}
	
	else if (Property)
	{
		SerializeGenericStruct(Property, ValuePtr, Ar);
	}
}

void UConstituent::InternalServerOnExecute(const uint8 InActionId, const TBitArray<>& SerializedParams)
{
	CurrentActionId = InActionId;
	CurrentParams = SerializedParams;
	SetBitReader(CurrentBitReader, CurrentParams);
	Server_OnExecute.Broadcast();
}

void UConstituent::InternalPredictedOnExecute(const uint8 InActionId, const bool bInIsReplaying,
                                              const bool bInIsFirstPerson, const bool bInIsServer, const TBitArray<>& SerializedParams)
{
	CurrentActionId = InActionId;
	bCurrentIsReplaying = bInIsReplaying;
	bCurrentIsFirstPerson = bInIsFirstPerson;
	bCurrentIsServer = bInIsServer;
	CurrentParams = SerializedParams;
	SetBitReader(CurrentBitReader, CurrentParams);
	Predicted_OnExecute.Broadcast();
}

void UConstituent::InternalAutonomousOnExecute(const uint8 InActionId, const float InTimeSinceExecution,
                                               const bool bInIsFirstPerson, const TBitArray<>& SerializedParams)
{
	CurrentActionId = InActionId;
	CurrentTimeSinceExecution = InTimeSinceExecution;
	bCurrentIsFirstPerson = bInIsFirstPerson;
	CurrentParams = SerializedParams;
	SetBitReader(CurrentBitReader, CurrentParams);
	Autonomous_OnExecute.Broadcast();
}

void UConstituent::InternalSimulatedOnExecute(const uint8 InActionId, const float InTimeSinceExecution,
                                              const bool bInIsFirstPerson, const TBitArray<>& SerializedParams)
{
	CurrentActionId = InActionId;
	CurrentTimeSinceExecution = InTimeSinceExecution;
	bCurrentIsFirstPerson = bInIsFirstPerson;
	CurrentParams = SerializedParams;
	SetBitReader(CurrentBitReader, CurrentParams);
	Simulated_OnExecute.Broadcast();
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

void UConstituent::NetMulticastClientPerformActionSet_Implementation(const FActionSet& InActionSet,
                                                                     const float InServerFormTimestamp)
{
	if (HasAuthority()) return;
	LastActionSet = InActionSet;
	LastActionSetTimestamp = InServerFormTimestamp;
	InternalClientPerformActionSet();
}

void UConstituent::InternalClientPerformActionSet()
{
	if (!FormCore)
	{
		UE_LOG(LogSfCore, Error,
		       TEXT(
			       "InternalClientPerformActionSet was called on UConstituent class %s without a missing FormCoreComponent."
		       ), *GetClass()->GetName());
		return;
	}
	if (!GetOwner())
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("InternalClientPerformActionSet was called on UConstituent class %s without a valid owner."),
		       *GetClass()->GetName());
		return;
	}
	float TimeSinceExecution = FMath::Max(FormCore->CalculateTimeSinceServerFormTimestamp(LastActionSetTimestamp), 0.f);
	//We zero out minimal differences to prevent subsequent nodes from making unnecessary calculations.
	if (TimeSinceExecution < 0.05)
	{
		TimeSinceExecution = 0.f;
	}
	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		for (const TPair<uint8, TBitArray<>>& Pair : LastActionSet.ToMap())
		{
			if (Autonomous_OnExecute.IsBound())
			{
				InternalAutonomousOnExecute(Pair.Key, TimeSinceExecution, FormCore->IsFirstPerson(), Pair.Value);
			}
		}
	}
	if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		for (const TPair<uint8, TBitArray<>>& Pair : LastActionSet.ToMap())
		{
			if (Simulated_OnExecute.IsBound())
			{
				InternalSimulatedOnExecute(Pair.Key, TimeSinceExecution, FormCore->IsFirstPerson(), Pair.Value);
			}
		}
	}
}

void UConstituent::IncrementTimeSincePredictedLastActionSet(const float InTimePassed)
{
	TimeSincePredictedLastActionSet.SetFloat(TimeSincePredictedLastActionSet.GetFloat() + InTimePassed);
}

void UConstituent::PerformLastActionSetOnClients()
{
	if (!HasAuthority())
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("UConstituent of class %s tried to call PerformLastActionSetsOnClients without authority."),
		       *GetClass()->GetName());
		return;
	}
	NetMulticastClientPerformActionSet(LastActionSet, FormCore->GetNonCompensatedServerFormTime());
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
	const UFormCharacterComponent* FormCharacter = FormCore->FormCharacter;
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
	                                  FormCharacter->CalculateFuturePredictedTimestamp(InTimeout), EventToBind));
	//We check buffed inputs instantly just in case they can be run already.
	OwningSlotable->OwningInventory->UpdateAndRunBufferedInputs(this);
}

void UConstituent::HandleBufferInputTimeout()
{
	const UFormCharacterComponent* FormCharacter = FormCore->FormCharacter;
	for (auto It = BufferedInputs.CreateIterator(); It; ++It)
	{
		if (FormCharacter->CalculateTimeUntilPredictedTimestamp(It->LifetimePredictedTimestamp) < 0)
		{
			It.RemoveCurrent();
		}
	}
}

UFormCoreComponent* UConstituent::GetFormCoreComponent() const
{
	if (!FormCore)
	{
		UE_LOG(LogSfCore, Error, TEXT("UConstituent of class %s does not have FormCoreComponent."),
		       *GetClass()->GetName())
	}
	return FormCore;
}

int32 UConstituent::GetCurrentActionId() const
{
	return CurrentActionId;
}

bool UConstituent::GetCurrentIsReplaying() const
{
	return bCurrentIsReplaying;
}

bool UConstituent::GetCurrentIsFirstPerson() const
{
	return bCurrentIsFirstPerson;
}

bool UConstituent::GetCurrentIsServer() const
{
	return bCurrentIsServer;
}

float UConstituent::GetCurrentTimeSinceExecution() const
{
	return CurrentTimeSinceExecution;
}

void UConstituent::GetCurrentParams(UConstituent* Target, UStruct*& OutStruct)
{
	UE_LOG(LogSfCore, Error,
		   TEXT(
			   "Normal implementation of ExecuteActionWithParams should not be called. Check if the thunk is implemented properly."
		   ));
}

TBitArray<> BitWriterToBitArray(const FBitWriter& Source)
{
	TBitArray<> BitArray;
	BitArray.SetNumUninitialized(Source.GetNumBits());
	FMemory::Memcpy(&BitArray, Source.GetData(), Source.GetNumBytes());
	return BitArray;
}

void SetBitReader(FBitReader& Reader, TBitArray<>& Source)
{
	Reader.SetData(reinterpret_cast<uint8*>(Source.GetData()), Source.Num());
}
