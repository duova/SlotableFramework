// Fill out your copyright notice in the Description page of Project Settings.


#include "Constituent.h"

#include "FormCharacterComponent.h"
#include "FormCoreComponent.h"
#include "Net/UnrealNetwork.h"

FSfPredictionFlags::FSfPredictionFlags(const bool bClientStatesWereCorrected, const bool bClientMetadataWasCorrected,
                                       const bool bClientOnPlayback, const bool bIsServer):
	bClientStatesWereCorrected(bClientStatesWereCorrected), bClientMetadataWasCorrected(bClientMetadataWasCorrected),
	bClientOnPlayback(bClientOnPlayback),
	bIsServer(bIsServer)
{
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
	if (TimeSinceStateChange < 65534 - DeltaTime * 100.0)
	{
		TimeSinceStateChange += DeltaTime * 100.0;
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
	DOREPLIFETIME_WITH_PARAMS_FAST(UConstituent, ConstituentState, DefaultParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UConstituent, TimeSinceStateChange, DefaultParams);
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
	//Call events.
}

bool UConstituent::ChangeConstituentState(const uint8 From, const uint8 To)
{
	if (ConstituentState != From)
	{
		return false;
	}
	if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy) return false;
	if (HasAuthority())
	{
		ConstituentState = To;
		Server_OnEnterConstituentState(ConstituentState);
		TimeSinceStateChange = 0;
		MARK_PROPERTY_DIRTY_FROM_NAME(UConstituent, ConstituentState, this);
		//We only mark this dirty when we change state.
		MARK_PROPERTY_DIRTY_FROM_NAME(UConstituent, TimeSinceStateChange, this);
	}
	if (bEnableInputsAndPrediction && IsFormCharacter())
	{
		//Predicted_ is run on server and client.
		//Don't have to cache FormCharacter because getter already caches.
		if (HasAuthority())
		{
			PredictedConstituentState = To;
			//On server the node implementations are to just run whatever is necessary on the server, like movement.
			Predicted_OnEnterConstituentState(ConstituentState, FSfPredictionFlags(false, false, false, true));
		}
		else
		{
			if (!GetFormCharacter()->bClientStatesWereCorrected && GetFormCharacter()->bClientOnPlayback)
			{
				//If the state was not rolled back, we should not resimulate state changes on playback, as everything
				//should be working correctly in actual client time.
				return false;
			}
			//If we are aren't playing back, we want to predict the state change.
			//If we are playing back and the states were corrected, we want to repredict those states and their reactors.
			PredictedConstituentState = To;
			//Don't have to cache FormCharacter because getter already caches.
			Predicted_OnEnterConstituentState(ConstituentState,
			                                  FSfPredictionFlags(GetFormCharacter()->bClientStatesWereCorrected,
			                                                     GetFormCharacter()->bClientMetadataWasCorrected,
			                                                     GetFormCharacter()->bClientOnPlayback, false));
		}
	}
	if (ConstituentState == To)
	{
		return true;
	}
	//Other roles change their states OnRep.
	return false;
}

void UConstituent::OnRep_ConstituentState()
{
	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		Autonomous_OnEnterConstituentState(ConstituentState, static_cast<float>(TimeSinceStateChange) / 100.0);
	}
	if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		if (OwningSlotable->OwningInventory->OwningFormCore->IsFirstPerson())
		{
			SimulatedFP_OnEnterConstituentState(ConstituentState, static_cast<float>(TimeSinceStateChange) / 100.0);
		}
		else
		{
			SimulatedTP_OnEnterConstituentState(ConstituentState, static_cast<float>(TimeSinceStateChange) / 100.0);
		}
	}
}
