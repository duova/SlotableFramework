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

FCard::FCard(const TSubclassOf<UCardObject>& InCardClass, const ECardType InCardType,
             const uint8 InOwnerConstituentInstanceId,
             const UFormCharacterComponent* InNullUnlessUsingPredictedTimestampFormCharacter,
             const UFormCoreComponent* InNullUnlessUsingServerTimestampFormCore, const float InCustomLifetime):
	Class(InCardClass),
	OwnerConstituentInstanceId(InOwnerConstituentInstanceId), bIsNotCorrected(0), bIsDisabledForDestroy(false),
	ServerAwaitClientSyncTimeoutTimestamp(0)
{
	const UCardObject* CardCDO = InCardClass.GetDefaultObject();

	//Get the deterministic index of the class.
	if (Class.Get())
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
		UE_LOG(LogSfCore, Error, TEXT("FCard has no CardObject."));
	}

	switch (InCardType)
	{
	case ECardType::DoNotUseLifetime:
		LifetimeEndTimestamp = -1;
		break;

	case ECardType::UseDefaultLifetimePredictedTimestamp:
		if (!CardCDO)
		{
			UE_LOG(LogSfCore, Error, TEXT("FCard could not get CardObject CDO."));
			break;
		}
		if (CardCDO->bUseLifetime)
		{
			bUsingPredictedTimestamp = true;
			if (!InNullUnlessUsingPredictedTimestampFormCharacter)
			{
				UE_LOG(LogSfCore, Error, TEXT("FCard constructor requires form character reference."));
				break;
			}
			LifetimeEndTimestamp = InNullUnlessUsingPredictedTimestampFormCharacter->
				CalculateFuturePredictedTimestamp(CardCDO->DefaultLifetimeInSeconds);
		}
		else
		{
			LifetimeEndTimestamp = -1;
		}
		break;

	case ECardType::UseDefaultLifetimeServerTimestamp:
		if (!CardCDO)
		{
			UE_LOG(LogSfCore, Error, TEXT("FCard could not get CardObject CDO."));
			break;
		}
		if (CardCDO->bUseLifetime)
		{
			bUsingPredictedTimestamp = false;
			if (!InNullUnlessUsingServerTimestampFormCore)
			{
				UE_LOG(LogSfCore, Error, TEXT("FCard constructor requires form core reference."));
				break;
			}
			LifetimeEndTimestamp = InNullUnlessUsingServerTimestampFormCore->CalculateFutureServerFormTimestamp(
				CardCDO->DefaultLifetimeInSeconds);
		}
		else
		{
			LifetimeEndTimestamp = -1;
		}
		break;

	case ECardType::UseCustomLifetimePredictedTimestamp:
		bUsingPredictedTimestamp = true;
		if (!InNullUnlessUsingPredictedTimestampFormCharacter)
		{
			UE_LOG(LogSfCore, Error, TEXT("FCard constructor requires form character reference."));
			break;
		}
		LifetimeEndTimestamp = InNullUnlessUsingPredictedTimestampFormCharacter->CalculateFuturePredictedTimestamp(
			InCustomLifetime);
		break;

	case ECardType::UseCustomLifetimeServerTimestamp:
		bUsingPredictedTimestamp = false;
		if (!InNullUnlessUsingServerTimestampFormCore)
		{
			UE_LOG(LogSfCore, Error, TEXT("FCard constructor requires form core reference."));
			break;
		}
		LifetimeEndTimestamp = InNullUnlessUsingServerTimestampFormCore->CalculateFutureServerFormTimestamp(InCustomLifetime);
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

bool FCard::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	//We have a list of all CardObject classes that is sorted by name deterministically so we only have to send the index.
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
	if (Ar.IsLoading())
	{
		Class = UFormCoreComponent::GetAllCardObjectClassesSortedByName()[ClassIndex];
	}
	Ar << OwnerConstituentInstanceId;
	Ar.SerializeBits(&bIsDisabledForDestroy, 1);
	//Don't serialize anything else if card is disabled, we don't need it.
	if (bIsDisabledForDestroy) return bOutSuccess;
	Ar.SerializeBits(&bUsingPredictedTimestamp, 1);
	Ar << LifetimeEndTimestamp;
	return bOutSuccess;
}
