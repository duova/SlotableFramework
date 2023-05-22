// Fill out your copyright notice in the Description page of Project Settings.


#include "Card.h"

#include "FormCharacterComponent.h"

FCard::FCard(): OwnerConstituentInstanceId(0), bUsingPredictedTimestamp(0), LifetimeEndTimestamp(0), bIsNotCorrected(0),
                bIsDisabledForDestroy(0),
                ClientSyncTimeRemaining(0)
{
}

FCard::FCard(const TSubclassOf<UCardObject>& CardClass, const ECardType CardType, const uint16 InOwnerConstituentInstanceId,
	UFormCharacterComponent* IfPredictedFormCharacter, UFormCoreComponent* IfServerFormCore, const float CustomLifetime):
	Class(CardClass),
	OwnerConstituentInstanceId(InOwnerConstituentInstanceId),
	bUsingPredictedTimestamp(0),
	LifetimeEndTimestamp(0),
	bIsNotCorrected(0),
	bIsDisabledForDestroy(0),
	ClientSyncTimeRemaining(0)
{
	const UCardObject* CardCDO = CardClass.GetDefaultObject();
	switch (CardType)
	{
	case ECardType::DoNotUseLifetime:
		LifetimeEndTimestamp = -1;
		break;

	case ECardType::UseDefaultLifetimePredictedTimestamp:
		if (CardCDO->bUseLifetime)
		{
			bUsingPredictedTimestamp = true;
			checkf(IfPredictedFormCharacter, TEXT("FCard constructor requires form character reference."));
			LifetimeEndTimestamp = IfPredictedFormCharacter->CalculateFuturePredictedTimestamp(CardCDO->DefaultLifetime);
		}
		else
		{
			LifetimeEndTimestamp = -1;
		}
		break;

	case ECardType::UseDefaultLifetimeServerTimestamp:
		if (CardCDO->bUseLifetime)
		{
			bUsingPredictedTimestamp = false;
			checkf(IfServerFormCore, TEXT("FCard constructor requires form core reference."));
			LifetimeEndTimestamp = IfServerFormCore->CalculateFutureServerTimestamp(CardCDO->DefaultLifetime);
		}
		else
		{
			LifetimeEndTimestamp = -1;
		}
		break;

	case ECardType::UseCustomLifetimePredictedTimestamp:
		bUsingPredictedTimestamp = true;
		checkf(IfPredictedFormCharacter, TEXT("FCard constructor requires form character reference."));
		LifetimeEndTimestamp = IfPredictedFormCharacter->CalculateFuturePredictedTimestamp(CustomLifetime);
		break;

	case ECardType::UseCustomLifetimeServerTimestamp:
		bUsingPredictedTimestamp = false;
		checkf(IfServerFormCore, TEXT("FCard constructor requires form core reference."));
		LifetimeEndTimestamp = IfServerFormCore->CalculateFutureServerTimestamp(CustomLifetime);
		break;
	}
}

bool FCard::operator==(const FCard& Other) const
{
	if (Class == Other.Class && OwnerConstituentInstanceId == Other.OwnerConstituentInstanceId && bUsingPredictedTimestamp == Other.bUsingPredictedTimestamp &&
		LifetimeEndTimestamp == Other.LifetimeEndTimestamp)
	{
		return true;
	}
	return false;
}

UCardObject::UCardObject()
{
}

void UCardObject::BeginDestroy()
{
	UObject::BeginDestroy();
}

void UCardObject::Initialize()
{
}

void UCardObject::Deinitialize()
{
}
