// Fill out your copyright notice in the Description page of Project Settings.


#include "FormCharacterComponent.h"

#include "CardObject.h"
#include "Inventory.h"
#include "Slotable.h"
#include "EnhancedInputComponent.h"
#include "FormStatComponent.h"
#include "GameFramework/Character.h"

FInventoryCards::FInventoryCards()
{
}

FInventoryCards::FInventoryCards(const TArray<FCard>& InCards)
{
	Cards = InCards;
}

bool FInventoryCards::operator==(const FInventoryCards& Other) const
{
	return Other.Cards == Cards;
}

bool FInventoryCards::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << Cards;
	return bOutSuccess;
}


FNetCardIdentifier::FNetCardIdentifier(): ClassIndex(0), OwnerConstituentInstanceId(0)
{
}

FNetCardIdentifier::FNetCardIdentifier(const uint16 InClassIndex, const uint8 InOwnerConstituentInstanceId):
	ClassIndex(InClassIndex), OwnerConstituentInstanceId(InOwnerConstituentInstanceId)
{
}

bool FNetCardIdentifier::operator==(const FNetCardIdentifier& Other) const
{
	return ClassIndex == Other.ClassIndex && OwnerConstituentInstanceId == Other.OwnerConstituentInstanceId;
}

bool FNetCardIdentifier::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	//We try to not send the full index if we don't have to.
	bool bClassIndexIsLarge = ClassIndex > 255;
	Ar.SerializeBits(&bClassIndexIsLarge, 1);
	if (bClassIndexIsLarge)
	{
		Ar << ClassIndex;
	}
	else
	{
		uint8 Value = ClassIndex;
		Ar << Value;
		ClassIndex = Value;
	}
	Ar << OwnerConstituentInstanceId;
	return bOutSuccess;
}

FCardIdentifiersInAnInventory::FCardIdentifiersInAnInventory()
{
}

FCardIdentifiersInAnInventory::FCardIdentifiersInAnInventory(const TArray<FNetCardIdentifier>& InCardIdentifiers)
{
	CardIdentifiers = InCardIdentifiers;
}

bool FCardIdentifiersInAnInventory::operator==(const FCardIdentifiersInAnInventory& Other) const
{
	return Other.CardIdentifiers == CardIdentifiers;
}

bool FCardIdentifiersInAnInventory::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << CardIdentifiers;
	return bOutSuccess;
}

FIdentifiedActionSet::FIdentifiedActionSet(): ConstituentInstanceId(0)
{
}

FIdentifiedActionSet::FIdentifiedActionSet(const uint16 InConstituentInstanceId, const FActionSet InActionSet):
	ConstituentInstanceId(InConstituentInstanceId), ActionSet(InActionSet)
{
}

bool FIdentifiedActionSet::operator==(const FIdentifiedActionSet& Other) const
{
	if (Other.ConstituentInstanceId == ConstituentInstanceId && Other.ActionSet == ActionSet) return true;
	return false;
}

bool FIdentifiedActionSet::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << ConstituentInstanceId;
	ActionSet.NetSerialize(Ar, Map, bOutSuccess);
	return bOutSuccess;
}

FSavedMove_Sf::FSavedMove_Sf()
	: bWantsToSprint(0), EnabledInputSets(0), PrimaryInputSet(0), SecondaryInputSet(0), TertiaryInputSet(0),
	  PredictedNetClock(0)
{
	//We force no combine as we want to send all moves as soon as possible to reduce input latency.
	bForceNoCombine = true;
}

void FSavedMove_Sf::Clear()
{
	FSavedMove_Character::Clear();

	bWantsToSprint = 0;
	EnabledInputSets = 0;
	PrimaryInputSet = 0;
	SecondaryInputSet = 0;
	TertiaryInputSet = 0;
	PredictedNetClock = 0;
	PendingActionSets.Empty();
	CardIdentifiersInInventories.Empty();
	Resources.Empty();
}

void FSavedMove_Sf::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel,
                               FNetworkPredictionData_Client_Character& ClientData)
{
	FSavedMove_Character::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	const UFormCharacterComponent* CharacterComponent = Cast<UFormCharacterComponent>(C->GetCharacterMovement());

	bWantsToSprint = CharacterComponent->bWantsToSprint;
	EnabledInputSets = CharacterComponent->EnabledInputSets;
	if (EnabledInputSets > 0)
	{
		PrimaryInputSet = CharacterComponent->PrimaryInputSet;
	}
	if (EnabledInputSets > 1)
	{
		SecondaryInputSet = CharacterComponent->SecondaryInputSet;
	}
	if (EnabledInputSets > 2)
	{
		TertiaryInputSet = CharacterComponent->TertiaryInputSet;
	}

	PredictedNetClock = CharacterComponent->PredictedNetClock;
	PendingActionSets = CharacterComponent->PendingActionSets;
	CardIdentifiersInInventories = CharacterComponent->CardIdentifiersInInventories;
	Resources = CharacterComponent->ClientSentResources;
}

void FSavedMove_Sf::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);

	UFormCharacterComponent* CharacterComponent = Cast<UFormCharacterComponent>(C->GetCharacterMovement());

	CharacterComponent->bWantsToSprint = bWantsToSprint;

	if (CharacterComponent->EnabledInputSets > 0)
	{
		CharacterComponent->PrimaryInputSet = PrimaryInputSet;
	}
	if (CharacterComponent->EnabledInputSets > 1)
	{
		CharacterComponent->SecondaryInputSet = SecondaryInputSet;
	}
	if (CharacterComponent->EnabledInputSets > 2)
	{
		CharacterComponent->TertiaryInputSet = TertiaryInputSet;
	}

	CharacterComponent->PredictedNetClock = PredictedNetClock;
	CharacterComponent->PendingActionSets = PendingActionSets;
	CharacterComponent->CardIdentifiersInInventories = CardIdentifiersInInventories;
	CharacterComponent->ClientSentResources = Resources;
}

uint8 FSavedMove_Sf::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	if (bWantsToSprint) Result |= FLAG_Custom_0;

	return Result;
}

FNetworkPredictionData_Client_Sf::FNetworkPredictionData_Client_Sf(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr FNetworkPredictionData_Client_Sf::AllocateNewMove()
{
	return MakeShared<FSavedMove_Sf>();
}

FSfNetworkMoveData::FSfNetworkMoveData()
	: EnabledInputSets(0), PrimaryInputSet(0), SecondaryInputSet(0), TertiaryInputSet(0),
	  PredictedNetClock(0)
{
}

bool FSfNetworkMoveData::Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar,
                                   UPackageMap* PackageMap, ENetworkMoveType MoveType)
{
	UFormCharacterComponent* CharacterComponent = Cast<UFormCharacterComponent>(&CharacterMovement);
	
	NetworkMoveType = MoveType;

	bool bLocalSuccess = true;
	const bool bIsSaving = Ar.IsSaving();

	Ar << TimeStamp;

	Acceleration.NetSerialize(Ar, PackageMap, bLocalSuccess);

	Location.NetSerialize(Ar, PackageMap, bLocalSuccess);

	// ControlRotation : FRotator handles each component zero/non-zero test; it uses a single signal bit for zero/non-zero, and uses 16 bits per component if non-zero.
	ControlRotation.NetSerialize(Ar, PackageMap, bLocalSuccess);

	SerializeOptionalValue<uint8>(bIsSaving, Ar, CompressedMoveFlags, 0);
	
	if (MoveType == ENetworkMoveType::NewMove)
	{
		// Location, relative movement base, and ending movement mode is only used for error checking, so only save for the final move.
		SerializeOptionalValue<UPrimitiveComponent*>(bIsSaving, Ar, MovementBase, nullptr);
		SerializeOptionalValue<FName>(bIsSaving, Ar, MovementBaseBoneName, NAME_None);
		SerializeOptionalValue<uint8>(bIsSaving, Ar, MovementMode, MOVE_Walking);
	}
	
	//We add our flags to serialization.
	Ar.SerializeBits(&EnabledInputSets, 2);
	if (EnabledInputSets > 0)
	{
		SerializeOptionalValue<uint8>(bIsSaving, Ar, PrimaryInputSet, 0);
	}
	if (EnabledInputSets > 1)
	{
		SerializeOptionalValue<uint8>(bIsSaving, Ar, SecondaryInputSet, 0);
	}
	if (EnabledInputSets > 2)
	{
		SerializeOptionalValue<uint8>(bIsSaving, Ar, TertiaryInputSet, 0);
	}

	//Conditionally serialize the clock to only send every second.
	bool bDoSerializeClock = bIsSaving && static_cast<float>(CharacterComponent->NetClockNextInteger) <
		PredictedNetClock;
	Ar.SerializeBits(&bDoSerializeClock, 1);
	if (bDoSerializeClock)
	{
		Ar << PredictedNetClock;
		if (bIsSaving)
		{
			CharacterComponent->NetClockNextInteger = floor(PredictedNetClock) + 1;
		}
	}
	else if (!bIsSaving)
	{
		//Negative values mean do not check on server.
		PredictedNetClock = -1;
	}
	
	//Only serialize pending action sets if actions were made that frame.
	bool bDoSerializeActionSets = bIsSaving && !PendingActionSets.IsEmpty();
	Ar.SerializeBits(&bDoSerializeActionSets, 1);
	if (bDoSerializeActionSets)
	{
		Ar << PendingActionSets;
	}
	
	//Conditionally serialize InventoryCardIdentifiers if they have been changed.
	bool bDoSerializeCardIdentifiers = bIsSaving && CardIdentifiersInInventories != CharacterComponent->
		OldCardIdentifiersInInventories;
	Ar.SerializeBits(&bDoSerializeCardIdentifiers, 1);
	if (bDoSerializeCardIdentifiers)
	{
		Ar << CardIdentifiersInInventories;
		//We set update OldInventoryCardIdentifiers on both the server and client.
		//The client version is what we use to check if we have attempted to send our latest changes already.
		//The server version is what is used if the client indicates they have no updates.
		CharacterComponent->OldCardIdentifiersInInventories = CardIdentifiersInInventories;
	}
	else if (!bIsSaving)
	{
		CardIdentifiersInInventories = CharacterComponent->OldCardIdentifiersInInventories;
	}

	//Only if FormResource is available on the form.
	if (CharacterComponent->FormResource)
	{
		//Conditionally serialize Resources if they have been changed.
		bool bDoSerializeResources = bIsSaving && Resources != CharacterComponent->
			OldClientSentResources;
		Ar.SerializeBits(&bDoSerializeResources, 1);
		if (bDoSerializeResources)
		{
			Ar << Resources;
			//We set update OldClientSentResources on both the server and client.
			//The client version is what we use to check if we have attempted to send our latest changes already.
			//The server version is what is used if the client indicates they have no updates.
			CharacterComponent->OldClientSentResources = Resources;
		}
		else if (!bIsSaving)
		{
			Resources = CharacterComponent->OldClientSentResources;
		}
	}

	return !Ar.IsError();
}

void FSfNetworkMoveData::ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType)
{
	FCharacterNetworkMoveData::ClientFillNetworkMoveData(ClientMove, MoveType);

	const FSavedMove_Sf* SavedMove = static_cast<const FSavedMove_Sf*>(&ClientMove);

	//Copy additional inputs.
	EnabledInputSets = SavedMove->EnabledInputSets;
	if (SavedMove->EnabledInputSets > 0)
	{
		PrimaryInputSet = SavedMove->PrimaryInputSet;
	}
	if (SavedMove->EnabledInputSets > 1)
	{
		SecondaryInputSet = SavedMove->SecondaryInputSet;
	}
	if (SavedMove->EnabledInputSets > 2)
	{
		TertiaryInputSet = SavedMove->TertiaryInputSet;
	}

	PredictedNetClock = SavedMove->PredictedNetClock;
	PendingActionSets = SavedMove->PendingActionSets;
	CardIdentifiersInInventories = SavedMove->CardIdentifiersInInventories;
	Resources = SavedMove->Resources;
}

FSfNetworkMoveDataContainer::FSfNetworkMoveDataContainer()
{
	NewMoveData = &NewSfMove;
	PendingMoveData = &PendingSfMove;
	OldMoveData = &OldSfMove;
}

FSfMoveResponseDataContainer::FSfMoveResponseDataContainer(): PredictedNetClock(0)
{
}

void FSfMoveResponseDataContainer::ServerFillResponseData(const UCharacterMovementComponent& CharacterMovement,
                                                          const FClientAdjustment& PendingAdjustment)
{
	FCharacterMoveResponseDataContainer::ServerFillResponseData(CharacterMovement, PendingAdjustment);

	const UFormCharacterComponent* CharacterComponent = Cast<UFormCharacterComponent>(&CharacterMovement);

	PredictedNetClock = CharacterComponent->PredictedNetClock;
	ActionSetResponse = CharacterComponent->ActionSetResponses;
	TimeSinceLastAction = CharacterComponent->TimesSinceLastAction;
	CardResponse = CharacterComponent->CardResponse;
	ResourcesResponse = CharacterComponent->ResourcesResponse;
}

void FSfMoveResponseDataContainer::SerializeCardResponse(FArchive& Ar, UFormCharacterComponent* CharacterComponent,
                                                         const bool bIsSaving)
{
	Ar << CardResponse;
	//We update OldInventoryCardIdentifiers on the client because we do not want it to immediately send the
	//correction back to the server. We don't update InventoryCardIdentifiers because it will just get overwritten.
	if (!bIsSaving)
	{
		CharacterComponent->bClientCardsWereUpdated = true;
		CharacterComponent->bMovementSpeedNeedsRecalculation = true;
		CharacterComponent->OldCardIdentifiersInInventories.Empty();
		for (FInventoryCards& InventoryCard : CardResponse)
		{
			//Place new element
			const uint16 i = CharacterComponent->OldCardIdentifiersInInventories.Emplace();
			//Reserve and add elements to the element.
			CharacterComponent->OldCardIdentifiersInInventories[i].CardIdentifiers.Reserve(InventoryCard.Cards.Num());
			for (FCard& Card : InventoryCard.Cards)
			{
				CharacterComponent->OldCardIdentifiersInInventories[i].CardIdentifiers.Emplace(
					Card.ClassIndex, Card.OwnerConstituentInstanceId);
			}
		}
	}
}

bool FSfMoveResponseDataContainer::Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar,
                                             UPackageMap* PackageMap)
{
	UFormCharacterComponent* CharacterComponent = Cast<UFormCharacterComponent>(&CharacterMovement);

	bool bLocalSuccess = true;
	const bool bIsSaving = Ar.IsSaving();

	Ar.SerializeBits(&ClientAdjustment.bAckGoodMove, 1);
	Ar << ClientAdjustment.TimeStamp;

	//We need the flags to know what to do after we receive.
	Ar.SerializeBits(&CharacterComponent->CorrectionConditionFlags, 5);

	if (!IsCorrection())
	{
		//We still want to be able to update cards without rollback if it's not a correction
		if ((CharacterComponent->CorrectionConditionFlags & Update_Cards) != 0)
		{
			SerializeCardResponse(Ar, CharacterComponent, bIsSaving);
		}
	}
	else
	{
		Ar.SerializeBits(&bHasBase, 1);
		Ar.SerializeBits(&bHasRotation, 1);
		Ar.SerializeBits(&bRootMotionMontageCorrection, 1);
		Ar.SerializeBits(&bRootMotionSourceCorrection, 1);

		//Movement must still always be repredicted on a correction because it's not exposed.
		ClientAdjustment.NewLoc.NetSerialize(Ar, PackageMap, bLocalSuccess);
		ClientAdjustment.NewVel.NetSerialize(Ar, PackageMap, bLocalSuccess);

		//We add our variables to serialization.

		//Conditionally serialize net clock.
		//We need the net clock to repredict action sets and cards as they are interlinked.
		bool bDoSerializeClock = (CharacterComponent->CorrectionConditionFlags
			& (Repredict_NetClock | Repredict_ActionSetsCardsResources)) != 0;
		if (bDoSerializeClock)
		{
			Ar << CharacterComponent->PredictedNetClock;
		}
		//Otherwise we leave it as is and don't increment in rollback.

		//Conditionally serialize Cards.
		//We serialize if Update_Cards or Repredict_ActionSetsCardsResources is true.
		bool bDoSerializeCards = (CharacterComponent->CorrectionConditionFlags
			& (Update_Cards | Repredict_ActionSetsCardsResources)) != 0;
		if (bDoSerializeCards)
		{
			SerializeCardResponse(Ar, CharacterComponent, bIsSaving);
		}

		//Conditionally serialize Resources.
		bool bDoSerializeResources = (CharacterComponent->CorrectionConditionFlags
			& Repredict_ActionSetsCardsResources) != 0;
		if (bDoSerializeResources)
		{
			Ar << CharacterComponent->ResourcesResponse;
			if (!bIsSaving)
			{
				CharacterComponent->bClientResourcesWereUpdated = true;
			}
		}

		//Conditionally serialize ActionSets.
		bool bDoSerializeActionSets = (CharacterComponent->CorrectionConditionFlags
			& Repredict_ActionSetsCardsResources) != 0;
		if (bDoSerializeActionSets)
		{
			Ar << CharacterComponent->ActionSetResponses;
			Ar << CharacterComponent->TimesSinceLastAction;
			if (!bIsSaving)
			{
				CharacterComponent->bClientActionsWereUpdated = true;
			}
		}

		if (bHasRotation)
		{
			ClientAdjustment.NewRot.NetSerialize(Ar, PackageMap, bLocalSuccess);
		}
		else if (!bIsSaving)
		{
			ClientAdjustment.NewRot = FRotator::ZeroRotator;
		}

		SerializeOptionalValue<UPrimitiveComponent*>(bIsSaving, Ar, ClientAdjustment.NewBase, nullptr);
		SerializeOptionalValue<FName>(bIsSaving, Ar, ClientAdjustment.NewBaseBoneName, NAME_None);
		SerializeOptionalValue<uint8>(bIsSaving, Ar, ClientAdjustment.MovementMode, MOVE_Walking);
		Ar.SerializeBits(&ClientAdjustment.bBaseRelativePosition, 1);

		if (bRootMotionMontageCorrection)
		{
			Ar << RootMotionTrackPosition;
		}
		else if (!bIsSaving)
		{
			RootMotionTrackPosition = -1.0f;
		}

		if (bRootMotionSourceCorrection)
		{
			if (FRootMotionSourceGroup* RootMotionSourceGroup = GetRootMotionSourceGroup(CharacterMovement))
			{
				RootMotionSourceGroup->NetSerialize(Ar, PackageMap, bLocalSuccess,
				                                    3 /*MaxNumRootMotionSourcesToSerialize*/);
			}
		}

		if (bRootMotionMontageCorrection || bRootMotionSourceCorrection)
		{
			RootMotionRotation.NetSerialize(Ar, PackageMap, bLocalSuccess);
		}
	}

	if (bIsSaving)
	{
		//We reset flags after serialization for the next frame.
		CharacterComponent->CorrectionConditionFlags = 0;
	}
	return !Ar.IsError();
}

UFormCharacterComponent::UFormCharacterComponent()
{
	SetNetworkMoveDataContainer(SfNetworkMoveDataContainer);
	SetMoveResponseDataContainer(SfMoveResponseDataContainer);
	bWantsToSprint = false;
	bMovementSpeedNeedsRecalculation = true;
	EnabledInputSets = 0;
	bClientActionsWereUpdated = false;
	bClientCardsWereUpdated = false;
	bClientResourcesWereUpdated = false;
}

bool UFormCharacterComponent::ShouldUsePackedMovementRPCs() const
{
	//We force use packed movement RPCs as this implementation requires it.
	return true;
}

FNetworkPredictionData_Client* UFormCharacterComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr)

	if (ClientPredictionData == nullptr)
	{
		//We have to const_cast to mutable to implement our own FNetworkPredictionData_Client.
		UFormCharacterComponent* MutableThis = const_cast<UFormCharacterComponent*>(this);

		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Sf(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
	}
	return ClientPredictionData;
}

void UFormCharacterComponent::ClientAdjustPosition_Implementation(float TimeStamp, FVector NewLoc, FVector NewVel,
                                                                  UPrimitiveComponent* NewBase, FName NewBaseBoneName,
                                                                  bool bHasBase, bool bBaseRelativePosition,
                                                                  uint8 ServerMovementMode,
                                                                  TOptional<FRotator> OptionalRotation)
{
	Super::ClientAdjustPosition_Implementation(TimeStamp, NewLoc, NewVel, NewBase, NewBaseBoneName, bHasBase,
	                                           bBaseRelativePosition, ServerMovementMode,
	                                           OptionalRotation);

	PredictedNetClock = SfMoveResponseDataContainer.PredictedNetClock;
	ActionSetResponses = SfMoveResponseDataContainer.ActionSetResponse;
	TimesSinceLastAction = SfMoveResponseDataContainer.TimeSinceLastAction;
	CardResponse = SfMoveResponseDataContainer.CardResponse;
	ResourcesResponse = SfMoveResponseDataContainer.ResourcesResponse;
}

void UFormCharacterComponent::MarkCardsDirty()
{
	CorrectionConditionFlags |= Update_Cards;
}

bool UFormCharacterComponent::IsReplaying() const
{
	return ClientPredictionData && ClientPredictionData->bUpdatePosition;
}

void UFormCharacterComponent::SetupFormCharacter(UFormCoreComponent* FormCoreComponent)
{
	FormCore = FormCoreComponent;

	//Sanity check to ensure only 24 slotable inputs are available.
	if (InputActionRegistry.Num() > 24)
	{
		UE_LOG(LogSfCore, Error,
		       TEXT(
			       "Only 24 input actions can be registered in UFormCharacterComponent class %s, removing additional ones."
		       ), *GetClass()->GetName());
		const uint16 CountToRemove = InputActionRegistry.Num() - 24;
		InputActionRegistry.RemoveAt(24, CountToRemove);
	}

	//Enable input sets depending on how many are necessary.
	if (InputActionRegistry.Num() < 9)
	{
		EnabledInputSets = 1;
	}
	else if (InputActionRegistry.Num() < 17)
	{
		EnabledInputSets = 2;
	}
	else
	{
		EnabledInputSets = 3;
	}
}

void UFormCharacterComponent::SecondarySetupFormCharacter()
{
	FormStat = FormCore->GetFormStat();
	FormResource = FormCore->GetFormResource();
	if (!FormResource) return;
	TimeBetweenResourceUpdate = FormResource->CalculatedTimeToEachResourceUpdate;
	ResourceUpdatesPerSecond = FormResource->OwnerResourceUpdateFrequencyPerSecond;
}

float UFormCharacterComponent::CalculateFuturePredictedTimestamp(const float InAdditionalTime) const
{
	//Read implementation of CalculateTimeUntilPredictedTimestamp for explanation of how this works.
	const float FutureTime = PredictedNetClock + InAdditionalTime;
	if (FutureTime < 40000.0) return FutureTime;
	return FutureTime - 40000.0;
}

float UFormCharacterComponent::CalculateTimeUntilPredictedTimestamp(const float InTimestamp) const
{
	//We treat the PredictedNetClock variable as a loop, and we assume if a point in the cycle is closer going forward, then
	//it is ahead in time and vice versa if it is closer going backwards.
	const float TimeToFirstTimestamp = PredictedNetClock > InTimestamp ? InTimestamp : PredictedNetClock;
	const float TimeBetween = FMath::Abs(PredictedNetClock - InTimestamp);
	const float TimeAfterSecondTimestamp = PredictedNetClock > InTimestamp
		                                       ? 40000.0 - PredictedNetClock
		                                       : InTimestamp - PredictedNetClock;
	const float TimeExterior = TimeToFirstTimestamp + TimeAfterSecondTimestamp;
	if (TimeExterior > TimeBetween)
	{
		return PredictedNetClock > InTimestamp ? -TimeBetween : TimeBetween;
	}
	return PredictedNetClock > InTimestamp ? TimeExterior : -TimeExterior;
}

float UFormCharacterComponent::CalculateTimeSincePredictedTimestamp(const float InTimestamp) const
{
	return -CalculateTimeUntilPredictedTimestamp(InTimestamp);
}

bool UFormCharacterComponent::HasPredictedTimestampPassed(const float InTimestamp) const
{
	return CalculateTimeUntilPredictedTimestamp(InTimestamp) <= 0;
}

void UFormCharacterComponent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
		//We bind all the inputs to the same function as we figure out which input triggered it with FInputActionInstance.
		for (const UInputAction* InputAction : InputActionRegistry)
		{
			if (!InputAction) continue;
			EnhancedInputComponent->BindAction(InputAction, ETriggerEvent::Started, this,
											   &UFormCharacterComponent::OnInputDown);
			EnhancedInputComponent->BindAction(InputAction, ETriggerEvent::Canceled, this,
											   &UFormCharacterComponent::OnInputUp);
		}
	}
}

void UFormCharacterComponent::OnInputDown(const FInputActionInstance& Instance)
{
	if (GetOwner()->GetLocalRole() != ROLE_AutonomousProxy) return;
	//Use the registry index to write to input sets.
	const uint8 InputIndex = FindInputIndexInRegistry(Instance);
	if (InputIndex == INDEX_NONE) return;
	if (InputIndex < 8)
	{
		SetInputSet(&PrimaryInputSet, InputIndex, true);
	}
	else if (InputIndex < 16)
	{
		SetInputSet(&SecondaryInputSet, InputIndex - 8, true);
	}
	else
	{
		SetInputSet(&TertiaryInputSet, InputIndex - 16, true);
	}
}

void UFormCharacterComponent::OnInputUp(const FInputActionInstance& Instance)
{
	if (GetOwner()->GetLocalRole() != ROLE_AutonomousProxy) return;
	//Use the registry index to write to input sets.
	const uint8 InputIndex = FindInputIndexInRegistry(Instance);
	if (InputIndex == INDEX_NONE) return;
	if (InputIndex < 8)
	{
		SetInputSet(&PrimaryInputSet, InputIndex, false);
	}
	else if (InputIndex < 16)
	{
		SetInputSet(&SecondaryInputSet, InputIndex - 8, false);
	}
	else
	{
		SetInputSet(&TertiaryInputSet, InputIndex - 16, false);
	}
}

void UFormCharacterComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetOwner()) return;
	if (!GetOwner()->FindComponentByClass(UFormCoreComponent::StaticClass()))
	{
		UE_LOG(LogSfCore, Error,
		       TEXT("FormCharacterComponent is not on an ACharacter with a FormCoreComponent, destroying ACharacter."));
		GetOwner()->Destroy();
	}
}

void UFormCharacterComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	//Get values from compressed flags.
	bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

bool UFormCharacterComponent::ServerCheckClientError(float ClientTimeStamp, float DeltaTime, const FVector& Accel,
                                                     const FVector& ClientWorldLocation,
                                                     const FVector& RelativeClientLocation,
                                                     UPrimitiveComponent* ClientMovementBase,
                                                     FName ClientBaseBoneName, uint8 ClientMovementMode)
{
	const bool bMovementErrored = Super::ServerCheckClientError(ClientTimeStamp, DeltaTime, Accel, ClientWorldLocation,
	                                                            RelativeClientLocation,
	                                                            ClientMovementBase,
	                                                            ClientBaseBoneName, ClientMovementMode);
	//We set our correction condition flags based on the context.
	if (bMovementErrored)
	{
		CorrectionConditionFlags |= Repredict_Movement;
	}
	SfServerCheckClientError();
	//We don't rollback if we only want to update cards, we only send changes with a good ack.
	return (CorrectionConditionFlags & ~Update_Cards) != 0;
}

void UFormCharacterComponent::MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags,
                                             const FVector& NewAccel)
{
	UpdateFromAdditionInputs();
	Super::MoveAutonomous(ClientTimeStamp, DeltaTime, CompressedFlags, NewAccel);
}

bool UFormCharacterComponent::CardHasEquivalentCardIdentifierFromClient(
	const FCardIdentifiersInAnInventory& InCardIdentifierInventoryFromClient,
	const FCard& InCard)
{
	for (const FNetCardIdentifier& CardIdentifier : InCardIdentifierInventoryFromClient.CardIdentifiers)
	{
		if (CardIdentifier.ClassIndex == InCard.ClassIndex && CardIdentifier.OwnerConstituentInstanceId == InCard.
			OwnerConstituentInstanceId)
		{
			return true;
		}
	}
	return false;
}

bool UFormCharacterComponent::CardCanBeFoundInInventory(UInventory* Inventory,
                                                        const FNetCardIdentifier InCardIdentifier)
{
	for (const FCard& Card : Inventory->Cards)
	{
		if (InCardIdentifier.ClassIndex == Card.ClassIndex && InCardIdentifier.OwnerConstituentInstanceId == Card.
			OwnerConstituentInstanceId)
		{
			return true;
		}
	}
	return false;
}

void UFormCharacterComponent::HandleInventoryDifferencesAndSetCorrectionFlags(
	UInventory* Inventory, const FCardIdentifiersInAnInventory& InCardIdentifierInventoryFromClient)
{
	TArray<FCard>& ServerCards = Inventory->Cards;
	for (int16 i = ServerCards.Num() - 1; i >= 0; i--)
	{
		if (CardHasEquivalentCardIdentifierFromClient(InCardIdentifierInventoryFromClient, ServerCards[i]))
		{
			//If the client has been updated with a server initiated card addition, we can start correcting that card.
			if (ServerCards[i].bIsNotCorrected)
			{
				ServerCards[i].bIsNotCorrected = false;
			}
		}
		else
		{
			if (ServerCards[i].bIsDisabledForDestroy)
			{
				//If the card is awaiting client sync to be destroyed, we destroy once the client acks that the card is
				//destroyed.
				ServerCards.RemoveAt(i, 1, false);
			}
			else
			{
				//Otherwise we should correct the client with the missing card.
				CorrectionConditionFlags |= Repredict_ActionSetsCardsResources;
			}
		}
	}
	ServerCards.Shrink();

	//Check if any card identifier is missing the equivalent card on the server. If so correct.
	for (const FNetCardIdentifier& CardIdentifier : InCardIdentifierInventoryFromClient.CardIdentifiers)
	{
		if (!CardCanBeFoundInInventory(Inventory, CardIdentifier))
		{
			CorrectionConditionFlags |= Repredict_ActionSetsCardsResources;
		}
	}
}

void UFormCharacterComponent::HandleResourcesDifferencesAndSetCorrectionFlags(
	const UFormResourceComponent* FormResourceComponent, const TArray<FResource>& InResources)
{
	const TArray<FResource>& ServerResources = FormResourceComponent->Resources;
	if (InResources.Num() != ServerResources.Num())
	{
		//If client sent resources array is a different size we must correct.
		CorrectionConditionFlags |= Repredict_ActionSetsCardsResources;
		return;
	}
	for (uint16 i = 0; i < ServerResources.Num(); i++)
	{
		if (ServerResources[i] != InResources[i])
		{
			CorrectionConditionFlags |= Repredict_ActionSetsCardsResources;
			return;
		}
	}
}

void UFormCharacterComponent::SfServerCheckClientError()
{
	const FSfNetworkMoveData* MoveData = static_cast<FSfNetworkMoveData*>(GetCurrentNetworkMoveData());

	//Don't check if negative.
	if (MoveData->PredictedNetClock >= 0)
	{
		//If not within acceptable tolerance we correct and repredict.
		if (MoveData->PredictedNetClock > PredictedNetClock + NetClockAcceptableTolerance
			|| MoveData->PredictedNetClock < PredictedNetClock - NetClockAcceptableTolerance)
		{
			CorrectionConditionFlags |= Repredict_NetClock;
		}
	}

	if (MoveData->PendingActionSets != PendingActionSets)
	{
		CorrectionConditionFlags |= Repredict_ActionSetsCardsResources;
	}

	//If there is a difference in the number of inventories, ie when the server added or removed an inventory and the
	//client hasn't synced up yet, we don't check cards since we can't with a index mismatch.
	const TArray<UInventory*> Inventories = FormCore->GetInventories();
	const TArray<FCardIdentifiersInAnInventory>& ClientCardIdentifiersInInventories = MoveData->
		CardIdentifiersInInventories;
	if (ClientCardIdentifiersInInventories.Num() != Inventories.Num())
	{
		return;
	}

	for (uint8 InventoryIndex = 0; InventoryIndex < Inventories.Num(); InventoryIndex++)
	{
		HandleInventoryDifferencesAndSetCorrectionFlags(Inventories[InventoryIndex],
		                                                ClientCardIdentifiersInInventories[InventoryIndex]);
	}

	if (FormResource)
	{
		HandleResourcesDifferencesAndSetCorrectionFlags(FormResource, MoveData->Resources);
	}
}

void UFormCharacterComponent::UpdateFromAdditionInputs()
{
	const FSfNetworkMoveData* MoveData = static_cast<FSfNetworkMoveData*>(GetCurrentNetworkMoveData());

	if (EnabledInputSets > 0)
	{
		PrimaryInputSet = MoveData->PrimaryInputSet;
	}
	if (EnabledInputSets > 1)
	{
		SecondaryInputSet = MoveData->SecondaryInputSet;
	}
	if (EnabledInputSets > 2)
	{
		TertiaryInputSet = MoveData->TertiaryInputSet;
	}
}

bool UFormCharacterComponent::ClientUpdatePositionAfterServerUpdate()
{
	const bool ReturnValue = Super::ClientUpdatePositionAfterServerUpdate();
	if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		//Reset correction condition flags after replay so we know when we're replaying and when we're not.
		CorrectionConditionFlags = 0;

		//Call a delegate so form components know to restart with their current state.
		if (OnPostRollback.IsBound())
		{
			OnPostRollback.Broadcast();
		}
	}
	return ReturnValue;
}

void UFormCharacterComponent::CorrectActionSets()
{
	//Constituent registry can't be used since this has to be in a deterministic order.
	uint16 i = 0;
	if (ActionSetResponses.Num() == 0 || TimesSinceLastAction.Num() == 0) return;
	for (const UInventory* Inventory : FormCore->GetInventories())
	{
		for (const USlotable* Slotable : Inventory->GetSlotables())
		{
			for (UConstituent* Constituent : Slotable->GetConstituents())
			{
				Constituent->PredictedLastActionSet = ActionSetResponses[i];
				Constituent->TimeSincePredictedLastActionSet = TimesSinceLastAction[i];
				i++;
			}
		}
	}
}

void UFormCharacterComponent::CorrectCards()
{
	const TArray<UInventory*>& Inventories = FormCore->GetInventories();
	for (uint16 i = 0; i < Inventories.Num(); i++)
	{
		Inventories[i]->Cards = CardResponse[i].Cards;
		Inventories[i]->ClientCheckAndUpdateCardObjects();
	}
}

void UFormCharacterComponent::CorrectResources() const
{
	FormResource->Resources = ResourcesResponse;
}

void UFormCharacterComponent::PackActionSets()
{
	ActionSetResponses.Empty();
	TimesSinceLastAction.Empty();
	ActionSetResponses.Reserve(FormCore->ConstituentRegistry.Num());
	TimesSinceLastAction.Reserve(FormCore->ConstituentRegistry.Num());
	//Constituent registry can't be used since this has to be in a deterministic order.
	for (const UInventory* Inventory : FormCore->GetInventories())
	{
		for (const USlotable* Slotable : Inventory->GetSlotables())
		{
			for (const UConstituent* Constituent : Slotable->GetConstituents())
			{
				ActionSetResponses.Add(Constituent->PredictedLastActionSet);
				TimesSinceLastAction.Add(Constituent->TimeSincePredictedLastActionSet);
			}
		}
	}
}

void UFormCharacterComponent::PackCards()
{
	CardResponse.Empty();
	CardResponse.Reserve(FormCore->GetInventories().Num());
	for (const UInventory* Inventory : FormCore->GetInventories())
	{
		FInventoryCards InventoryCards = FInventoryCards(Inventory->Cards);
		TArray<FCard>& InventoryCardsArray = InventoryCards.Cards;
		for (int16 i = InventoryCardsArray.Num() - 1; i >= 0; i--)
		{
			//We remove the cards that are disabled for destroy as we don't want those syncing to the client.
			if (InventoryCardsArray[i].bIsDisabledForDestroy)
			{
				InventoryCardsArray.RemoveAt(i, 1, false);
			}
		}
		InventoryCardsArray.Shrink();
		CardResponse.Add(InventoryCards);
	}
}

void UFormCharacterComponent::PackResources()
{
	ResourcesResponse = FormCore->GetFormResource()->Resources;
}

void UFormCharacterComponent::PackInventoryCardIdentifiers()
{
	CardIdentifiersInInventories.Empty();
	for (UInventory* Inventory : FormCore->GetInventories())
	{
		const uint8 i = CardIdentifiersInInventories.Emplace();
		for (FCard Card : Inventory->Cards)
		{
			CardIdentifiersInInventories[i].CardIdentifiers.Emplace(
				Card.ClassIndex, Card.OwnerConstituentInstanceId);
		}
	}
}

void UFormCharacterComponent::HandleCardClientSyncTimeout() const
{
	if (!GetOwner()) return;
	if (!GetOwner()->HasAuthority()) return;
	for (UInventory* Inventory : FormCore->GetInventories())
	{
		TArray<FCard>& Cards = Inventory->Cards;
		for (int16 i = Cards.Num() - 1; i >= 0; i--)
		{
			if (Cards[i].bIsNotCorrected && FormCore->HasServerFormTimestampPassed(
				Cards[i].ServerAwaitClientSyncTimeoutTimestamp))
			{
				//We set bIsNotCorrected to false so these cards get checked against the client ones and essentially
				//force a correction to get the client to sync up.
				Cards[i].bIsNotCorrected = false;
			}
			if (Cards[i].bIsDisabledForDestroy && FormCore->HasServerFormTimestampPassed(
				Cards[i].ServerAwaitClientSyncTimeoutTimestamp))
			{
				//We remove these cards which also forces a correction to get the client to sync up.
				Cards.RemoveAt(i, 1, false);
			}
		}
		Cards.Shrink();
	}
}

void UFormCharacterComponent::ApplyInputBitsToInventory(const uint32 InInputBits, const UInventory* InInventory)
{
	//Note that input indices 0-23 represent the index of the input action that is registered in the InputActionRegistry.
	const TArray<int8>& BoundInputIndices = InInventory->OrderedInputBindingIndices;
	for (uint8 i = 0; i < BoundInputIndices.Num(); i++)
	{
		//Check if input is actually bound.
		if (BoundInputIndices[i] == INDEX_NONE) continue;
		const bool bInputValue = GetValueFromInputSets(BoundInputIndices[i], InInputBits);
		//We don't want to do anything if the value isn't actually updated.
		if (InInventory->OrderedLastInputState[i] == bInputValue) continue;
		if (InInventory->Slotables.Num() <= i) continue;
		if (InInventory->Slotables[i] == nullptr) continue;
		for (UConstituent* Constituent : InInventory->Slotables[i]->GetConstituents())
		{
			if (!Constituent->bEnableInputsAndPrediction) continue;
			if (bInputValue)
			{
				Constituent->OnInputDown(true);
			}
			else
			{
				Constituent->OnInputUp(true);
			}
		}
	}
}

void UFormCharacterComponent::RemovePredictedCardWithEndedLifetimes() const
{
	for (UInventory* Inventory : FormCore->GetInventories())
	{
		TArray<FCard>& Cards = Inventory->Cards;
		for (int16 i = Cards.Num() - 1; i >= 0; i--)
		{
			if (Cards[i].bUsingPredictedTimestamp && Cards[i].LifetimeEndTimestamp > -1.f &&  CalculateTimeUntilPredictedTimestamp(Cards[i].LifetimeEndTimestamp)
				< 0)
			{
				Cards.RemoveAt(i, 1, false);
			}
		}
		Cards.Shrink();
	}
}

void UFormCharacterComponent::CalculateMovementSpeed()
{
	//Collect modifiers.
	float AdditiveMovementSpeedMultiplierSum = 0;
	//1 as the base to multiply into the modifier.
	float AdditiveMultiplicativeMovementSpeedModifierSum = 1;
	float TrueMultiplicativeMovementSpeedModifierProduct = 0;
	float FlatAdditiveMovementSpeedModifierSum = 0;

	for (const UInventory* Inventory : FormCore->GetInventories())
	{
		for (const FCard& Card : Inventory->Cards)
		{
			if (Card.bIsDisabledForDestroy) continue;
			const UCardObject* CardCDO = static_cast<UCardObject*>(Card.Class->ClassDefaultObject);
			if (CardCDO->bUseMovementSpeedModifiers) continue;
			AdditiveMovementSpeedMultiplierSum += CardCDO->AdditiveMovementSpeedModifier;
			AdditiveMultiplicativeMovementSpeedModifierSum += CardCDO->AdditiveMultiplicativeMovementSpeedModifier;
			TrueMultiplicativeMovementSpeedModifierProduct *= CardCDO->TrueMultiplicativeMovementSpeedModifier;
			FlatAdditiveMovementSpeedModifierSum += CardCDO->FlatAdditiveMovementSpeedModifier;

			//TrueMultiplicative needs to be checked for negative every iteration to avoid double negatives being missed.
			TrueMultiplicativeMovementSpeedModifierProduct = FMath::Clamp(
				TrueMultiplicativeMovementSpeedModifierProduct, 0.f, 999999.f);
		}
	}

	//Zero out negatives for multiplicative modifier.
	AdditiveMultiplicativeMovementSpeedModifierSum = FMath::Clamp(AdditiveMultiplicativeMovementSpeedModifierSum, 0.f,
	                                                              999999.f);

	//Apply modifiers if applicable.
	if (bWalkSpeedCalculatedByFormCharacter)
	{
		MaxWalkSpeed = BaseWalkSpeed;
		MaxWalkSpeed += AdditiveMovementSpeedMultiplierSum;
		MaxWalkSpeed *= AdditiveMultiplicativeMovementSpeedModifierSum;
		MaxWalkSpeed *= TrueMultiplicativeMovementSpeedModifierProduct;
		MaxWalkSpeed += FlatAdditiveMovementSpeedModifierSum;
		MaxWalkSpeed *= VariableWalkSpeedMultiplier;
	}

	if (bSwimSpeedCalculatedByFormCharacter)
	{
		MaxSwimSpeed = BaseSwimSpeed;
		MaxSwimSpeed += AdditiveMovementSpeedMultiplierSum;
		MaxSwimSpeed *= AdditiveMultiplicativeMovementSpeedModifierSum;
		MaxSwimSpeed *= TrueMultiplicativeMovementSpeedModifierProduct;
		MaxSwimSpeed += FlatAdditiveMovementSpeedModifierSum;
		MaxSwimSpeed *= VariableSwimSpeedMultiplier;
	}

	if (bFlySpeedCalculatedByFormCharacter)
	{
		MaxFlySpeed = BaseFlySpeed;
		MaxFlySpeed += AdditiveMovementSpeedMultiplierSum;
		MaxFlySpeed *= AdditiveMultiplicativeMovementSpeedModifierSum;
		MaxFlySpeed *= TrueMultiplicativeMovementSpeedModifierProduct;
		MaxFlySpeed += FlatAdditiveMovementSpeedModifierSum;
		MaxFlySpeed *= VariableFlySpeedMultiplier;
	}

	if (bAccelerationCalculatedByFormCharacter)
	{
		MaxAcceleration = BaseAcceleration;
		MaxAcceleration += AdditiveMovementSpeedMultiplierSum;
		MaxAcceleration *= AdditiveMultiplicativeMovementSpeedModifierSum;
		MaxAcceleration *= TrueMultiplicativeMovementSpeedModifierProduct;
		MaxAcceleration += FlatAdditiveMovementSpeedModifierSum;
		MaxAcceleration *= VariableAccelerationMultiplier;
	}

	if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		FormCore->WalkSpeedStat = MaxWalkSpeed;
		FormCore->SwimSpeedStat = MaxSwimSpeed;
		FormCore->FlySpeedStat = MaxFlySpeed;
		FormCore->AccelerationStat = MaxAcceleration;
		FormCore->SetMovementStatsDirty();
	}
}

void UFormCharacterComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	//On client we extract the corrected states from the FormCharacter if necessary.
	if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		if (bClientActionsWereUpdated)
		{
			bClientActionsWereUpdated = false;
			CorrectActionSets();
		}
		if (bClientCardsWereUpdated)
		{
			bClientCardsWereUpdated = false;
			CorrectCards();
		}
		if (FormResource && bClientResourcesWereUpdated)
		{
			bClientResourcesWereUpdated = false;
			CorrectResources();
		}
	}

	//If we're not replaying, or if we would like to repredict the net clock or action sets, cards, or resources.
	//Not if we want to repredict movement or update cards only.
	if (!IsReplaying() || (CorrectionConditionFlags & (Repredict_NetClock | Repredict_ActionSetsCardsResources)) != 0)
	{
		if (FormResource && FormStat)
		{
			//Apply consistent resource changes.
			//Applies every at a specified increment of the predicted net clock only to reduce bandwidth and processing usage.
			//This math is supposed to divide the clock by the time between resource updates and use FMath::Floor to determine whether
			//the second number has crossed the threshold. Multiplication is used instead for speed.
			if (FMath::Floor(PredictedNetClock * ResourceUpdatesPerSecond) != FMath::Floor(
				(PredictedNetClock + DeltaSeconds) * ResourceUpdatesPerSecond))
			{
				for (FResource& Resource : FormResource->Resources)
				{
					FormResource->LocalInternalAddResourceValue(
						Resource, FormStat->GetStat(Resource.IncreasePerSecondStat) * TimeBetweenResourceUpdate);
					FormResource->LocalInternalRemoveResourceValue(
						Resource, FormStat->GetStat(Resource.DecreasePerSecondStat) * TimeBetweenResourceUpdate);
				}
			}
		}

		PredictedNetClock += DeltaSeconds;
		//We reset when it is soon to lose 2 decimal points of precision.
		if (PredictedNetClock > 40000.0)
		{
			PredictedNetClock = 0;
		}

		RemovePredictedCardWithEndedLifetimes();

		//Run input if we're not replaying, or if we want to repredict anything that is not movement.
		const uint32 InputBits = GetInputSetsJoinedBits();
		for (const UInventory* Inventory : FormCore->GetInventories())
		{
			ApplyInputBitsToInventory(InputBits, Inventory);
		}

		//Remove timed out buffered inputs.
		for (UConstituent* Constituent : FormCore->ConstituentRegistry)
		{
			Constituent->HandleBufferInputTimeout();
			//Note that the buffered inputs firing is implemented with the addition or removal of cards.
		}

		//Pack the action sets back into the FormCharacter only after we've changed them.
		PendingActionSets.Empty();
		PendingActionSets.Reserve(FormCore->ConstituentRegistry.Num());
		for (UConstituent* Constituent : FormCore->ConstituentRegistry)
		{
			Constituent->IncrementTimeSincePredictedLastActionSet(DeltaSeconds);
			if (Constituent->bPredictedLastActionSetUpdated)
			{
				Constituent->bPredictedLastActionSetUpdated = false;
				PendingActionSets.Emplace(Constituent->InstanceId, Constituent->PredictedLastActionSet);
			}
		}

		if (FormResource)
		{
			PackResources();
		}

		if (GetOwner() && !GetOwner()->HasAuthority())
		{
			//We only create the list of card identifiers on the client, as on the server we directly compare them
			//to the actual cards in order to account for prediction skipping flags and to finalize server only card
			//changes if the client sends them back.
			PackInventoryCardIdentifiers();
		}
		else
		{
			//On server.
			//We handle cards where correction pausing flags have timed out.
			HandleCardClientSyncTimeout();

			//We only need to pack final action sets and cards on the server as they are only used for corrections.
			PackActionSets();
			PackCards();
		}
	}

	if (bMovementSpeedNeedsRecalculation)
	{
		CalculateMovementSpeed();
		bMovementSpeedNeedsRecalculation = false;
	}
}

uint8 UFormCharacterComponent::FindInputIndexInRegistry(const FInputActionInstance& InInstance)
{
	for (uint8 i = 0; i < InputActionRegistry.Num(); i++)
	{
		if (InInstance.GetSourceAction() == InputActionRegistry[i])
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void UFormCharacterComponent::SetInputSet(uint8* InputSet, const uint8 InIndex, const bool bInIsTrue)
{
	if (InIndex > 7)
	{
		UE_LOG(LogSfCore, Error, TEXT("Index for input set may not be larger than 7."));
		return;
	}
	const uint8 BitModifier = 1 << InIndex;
	if (bInIsTrue)
	{
		*InputSet |= BitModifier;
	}
	else
	{
		*InputSet &= ~BitModifier;
	}
}

bool UFormCharacterComponent::GetValueFromInputSets(const uint8 InIndex, const uint32 InInputSetsJoinedBits)
{
	if (InIndex > 23)
	{
		UE_LOG(LogSfCore, Error, TEXT("Index for input search may not be larger than 23."));
		return false;
	}
	const uint32 BitMask = 1 << InIndex;
	return (InInputSetsJoinedBits & BitMask) != 0;
}

uint32 UFormCharacterComponent::GetInputSetsJoinedBits() const
{
	return static_cast<uint32>(TertiaryInputSet) << 16 | static_cast<uint16>(SecondaryInputSet) << 8 | PrimaryInputSet;
}
