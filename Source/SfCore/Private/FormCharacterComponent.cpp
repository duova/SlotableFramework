// Fill out your copyright notice in the Description page of Project Settings.


#include "SfCore/Public/FormCharacterComponent.h"

#include "GameFramework/Character.h"

FSavedMove_Sf::FSavedMove_Sf()
	: Super(), bWantsToSprint(0), EnabledInputSets(0), PrimaryInputSet(0), SecondaryInputSet(0), TertiaryInputSet(0),
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
	ConstituentStates.Empty();
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
	ConstituentStates = CharacterComponent->ConstituentStates;
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
	CharacterComponent->ConstituentStates = ConstituentStates;
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
	return FSavedMovePtr(new FSavedMove_Sf);
}

FSfNetworkMoveData::FSfNetworkMoveData()
	: FCharacterNetworkMoveData(), EnabledInputSets(0), PrimaryInputSet(0), SecondaryInputSet(0), TertiaryInputSet(0),
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

	//We add our flags to serialization.
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

	//Conditionally serialize the clock.
	bool bDoSerializeClock = bIsSaving && static_cast<float>(CharacterComponent->NetClockNextInteger) < PredictedNetClock;
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

	//Conditionally serialize constituent states.
	bool bDoSerializeStates = bIsSaving && ConstituentStates != CharacterComponent->OldConstituentStates;
	Ar.SerializeBits(&bDoSerializeStates, 1);
	if (bDoSerializeStates)
	{
		Ar << ConstituentStates;
		//We set update OldConstituentStates on both the server and client.
		//The client version is what we use to check if we have attempted to send our latest changes already.
		//The server version is what is used if the client indicates they have no updates.
		CharacterComponent->OldConstituentStates = ConstituentStates;
	}
	else if (!bIsSaving)
	{
		ConstituentStates = CharacterComponent->OldConstituentStates;
	}

	if (MoveType == ENetworkMoveType::NewMove)
	{
		// Location, relative movement base, and ending movement mode is only used for error checking, so only save for the final move.
		SerializeOptionalValue<UPrimitiveComponent*>(bIsSaving, Ar, MovementBase, nullptr);
		SerializeOptionalValue<FName>(bIsSaving, Ar, MovementBaseBoneName, NAME_None);
		SerializeOptionalValue<uint8>(bIsSaving, Ar, MovementMode, MOVE_Walking);
	}

	return !Ar.IsError();
}

void FSfNetworkMoveData::ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType)
{
	FCharacterNetworkMoveData::ClientFillNetworkMoveData(ClientMove, MoveType);

	const FSavedMove_Sf* SavedMove = Cast<FSavedMove_Sf>(&ClientMove);

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
	ConstituentStates = SavedMove->ConstituentStates;
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
	ConstituentStates = CharacterComponent->ConstituentStates;
	TimeSinceStateChange = CharacterComponent->TimeSinceStateChange;
}

bool FSfMoveResponseDataContainer::Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar,
	UPackageMap* PackageMap)
{
	UFormCharacterComponent* CharacterComponent = Cast<UFormCharacterComponent>(&CharacterMovement);
	
	bool bLocalSuccess = true;
	const bool bIsSaving = Ar.IsSaving();

	Ar.SerializeBits(&ClientAdjustment.bAckGoodMove, 1);
	Ar << ClientAdjustment.TimeStamp;

	if (IsCorrection())
	{
		Ar.SerializeBits(&bHasBase, 1);
		Ar.SerializeBits(&bHasRotation, 1);
		Ar.SerializeBits(&bRootMotionMontageCorrection, 1);
		Ar.SerializeBits(&bRootMotionSourceCorrection, 1);

		ClientAdjustment.NewLoc.NetSerialize(Ar, PackageMap, bLocalSuccess);
		ClientAdjustment.NewVel.NetSerialize(Ar, PackageMap, bLocalSuccess);

		//We add our variables to serialization.
		Ar << CharacterComponent->PredictedNetClock;

		//Conditionally serialize constituent states.
		//Once again, we use OldConstituentStates to check that we do not send what is unnecessary.
		//We check against the last set of states sent by the client.
		bool bDoSerializeStates = bIsSaving && ConstituentStates != CharacterComponent->OldConstituentStates;
		Ar.SerializeBits(&bDoSerializeStates, 1);
		if (bDoSerializeStates)
		{
			Ar << CharacterComponent->ConstituentStates;
			Ar << CharacterComponent->TimeSinceStateChange;
			//We update OldConstituentData on the client and server because we do not want it to immediately send the
			//correction back to the server / send another correction.
			CharacterComponent->OldConstituentStates = ConstituentStates;
			if (!bIsSaving)
			{
				CharacterComponent->bClientStatesAreDirty = true;
			}
		}
		else if (!bIsSaving)
		{
			//The client version is what is used if the server indicates that there are no corrections.
			ConstituentStates = CharacterComponent->ConstituentStates;
			TimeSinceStateChange = TArray<uint16>();
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
				RootMotionSourceGroup->NetSerialize(Ar, PackageMap, bLocalSuccess, 3 /*MaxNumRootMotionSourcesToSerialize*/);
			}
		}

		if (bRootMotionMontageCorrection || bRootMotionSourceCorrection)
		{
			RootMotionRotation.NetSerialize(Ar, PackageMap, bLocalSuccess);
		}
	}

	return !Ar.IsError();
}

UFormCharacterComponent::UFormCharacterComponent()
{
	SetNetworkMoveDataContainer(SfNetworkMoveDataContainer);
	SetMoveResponseDataContainer(SfMoveResponseDataContainer);
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
	UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition,
	uint8 ServerMovementMode, TOptional<FRotator> OptionalRotation)
{
	Super::ClientAdjustPosition_Implementation(TimeStamp, NewLoc, NewVel, NewBase, NewBaseBoneName, bHasBase,
	                                           bBaseRelativePosition, ServerMovementMode,
	                                           OptionalRotation);
	
	PredictedNetClock = SfMoveResponseDataContainer.PredictedNetClock;
	ConstituentStates = SfMoveResponseDataContainer.ConstituentStates;
	TimeSinceStateChange = SfMoveResponseDataContainer.TimeSinceStateChange;
}

void UFormCharacterComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UFormCharacterComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	
	//Get values from compressed flags.
	bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

bool UFormCharacterComponent::ServerCheckClientError(float ClientTimeStamp, float DeltaTime, const FVector& Accel,
	const FVector& ClientWorldLocation, const FVector& RelativeClientLocation, UPrimitiveComponent* ClientMovementBase,
	FName ClientBaseBoneName, uint8 ClientMovementMode)
{
	return Super::ServerCheckClientError(ClientTimeStamp, DeltaTime, Accel, ClientWorldLocation, RelativeClientLocation,
	                                     ClientMovementBase,
	                                     ClientBaseBoneName, ClientMovementMode) || SfServerCheckClientError();
}

void UFormCharacterComponent::MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags,
	const FVector& NewAccel)
{
	UpdateFromAdditionInputs();
	Super::MoveAutonomous(ClientTimeStamp, DeltaTime, CompressedFlags, NewAccel);
}

bool UFormCharacterComponent::SfServerCheckClientError()
{
	const FSfNetworkMoveData* MoveData = static_cast<FSfNetworkMoveData*>(GetCurrentNetworkMoveData());
	
	if (MoveData->PredictedNetClock >= 0)
	{
		if (MoveData->PredictedNetClock > PredictedNetClock + NetClockAcceptableTolerance
			|| MoveData->PredictedNetClock < PredictedNetClock - NetClockAcceptableTolerance)
		{
			return true;
		}
	}
	
	if (MoveData->ConstituentStates != ConstituentStates) return true;

	return false;
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
