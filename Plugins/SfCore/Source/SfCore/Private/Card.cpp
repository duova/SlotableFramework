// Fill out your copyright notice in the Description page of Project Settings.


#include "Card.h"

#include "CardObject.h"
#include "FormCharacterComponent.h"

FCard::FCard(): ClassIndex(0), OwnerConstituentInstanceId(0), bUsingPredictedTimestamp(false), LifetimeEndTimestamp(0),
                bIsNotCorrected(0),
                bIsDisabledForDestroy(false),
                ServerAwaitClientSyncTimeoutTimestamp(0)
{
}

FCard::FCard(const TSubclassOf<UCardObject>& CardClass, const ECardType CardType,
             const uint8 InOwnerConstituentInstanceId,
             UFormCharacterComponent* NullUnlessUsingPredictedTimestampFormCharacter,
             UFormCoreComponent* NullUnlessUsingServerTimestampFormCore, const float CustomLifetime):
	Class(CardClass),
	OwnerConstituentInstanceId(InOwnerConstituentInstanceId), bIsNotCorrected(0), bIsDisabledForDestroy(false),
	ServerAwaitClientSyncTimeoutTimestamp(0)
{
	const UCardObject* CardCDO = CardClass.GetDefaultObject();

	//Get the deterministic index of the class.
	if (Class)
	{
		for (uint16 i = 0; i < UFormCoreComponent::GetAllCardObjectClassesSortedByName().Num(); i++)
		{
			if (UFormCoreComponent::GetAllCardObjectClassesSortedByName()[i] == Class.Get())
			{
				ClassIndex = i;
				break;
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("FCard has no CardObject."));
	}

	switch (CardType)
	{
	case ECardType::DoNotUseLifetime:
		LifetimeEndTimestamp = -1;
		break;

	case ECardType::UseDefaultLifetimePredictedTimestamp:
		if (!CardCDO)
		{
			UE_LOG(LogTemp, Error, TEXT("FCard could not get CardObject CDO."));
			break;
		}
		if (CardCDO->bUseLifetime)
		{
			bUsingPredictedTimestamp = true;
			if (!NullUnlessUsingPredictedTimestampFormCharacter)
			{
				UE_LOG(LogTemp, Error, TEXT("FCard constructor requires form character reference."));
				break;
			}
			LifetimeEndTimestamp = NullUnlessUsingPredictedTimestampFormCharacter->
				CalculateFuturePredictedTimestamp(CardCDO->DefaultLifetime);
		}
		else
		{
			LifetimeEndTimestamp = -1;
		}
		break;

	case ECardType::UseDefaultLifetimeServerTimestamp:
		if (!CardCDO)
		{
			UE_LOG(LogTemp, Error, TEXT("FCard could not get CardObject CDO."));
			break;
		}
		if (CardCDO->bUseLifetime)
		{
			bUsingPredictedTimestamp = false;
			if (!NullUnlessUsingServerTimestampFormCore)
			{
				UE_LOG(LogTemp, Error, TEXT("FCard constructor requires form core reference."));
				break;
			}
			LifetimeEndTimestamp = NullUnlessUsingServerTimestampFormCore->CalculateFutureServerTimestamp(
				CardCDO->DefaultLifetime);
		}
		else
		{
			LifetimeEndTimestamp = -1;
		}
		break;

	case ECardType::UseCustomLifetimePredictedTimestamp:
		bUsingPredictedTimestamp = true;
		if (!NullUnlessUsingPredictedTimestampFormCharacter)
		{
			UE_LOG(LogTemp, Error, TEXT("FCard constructor requires form character reference."));
			break;
		}
		LifetimeEndTimestamp = NullUnlessUsingPredictedTimestampFormCharacter->CalculateFuturePredictedTimestamp(
			CustomLifetime);
		break;

	case ECardType::UseCustomLifetimeServerTimestamp:
		bUsingPredictedTimestamp = false;
		if (!NullUnlessUsingServerTimestampFormCore)
		{
			UE_LOG(LogTemp, Error, TEXT("FCard constructor requires form core reference."));
			break;
		}
		LifetimeEndTimestamp = NullUnlessUsingServerTimestampFormCore->CalculateFutureServerTimestamp(CustomLifetime);
		break;
	}
}

FNetCardIdentifier FCard::GetNetCardIdentifier() const
{
	return FNetCardIdentifier(ClassIndex, OwnerConstituentInstanceId);
}

bool FCard::operator==(const FCard& Other) const
{
	return Class == Other.Class && OwnerConstituentInstanceId == Other.OwnerConstituentInstanceId &&
		bUsingPredictedTimestamp == Other.bUsingPredictedTimestamp &&
		LifetimeEndTimestamp == Other.LifetimeEndTimestamp;
}
