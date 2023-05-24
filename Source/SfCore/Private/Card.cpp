// Fill out your copyright notice in the Description page of Project Settings.


#include "Card.h"

#include "CardObject.h"
#include "FormCharacterComponent.h"

FCard::FCard(): ClassIndex(0), OwnerConstituentInstanceId(0), bUsingPredictedTimestamp(0), LifetimeEndTimestamp(0),
                bIsNotCorrected(0),
                bIsDisabledForDestroy(0),
                ServerAwaitClientSyncTimeoutTimestamp(0)
{
}

FCard::FCard(const TSubclassOf<UCardObject>& CardClass, const ECardType CardType, const uint8 InOwnerConstituentInstanceId,
             UFormCharacterComponent* IfPredictedFormCharacter, UFormCoreComponent* IfServerFormCore, const float CustomLifetime):
	Class(CardClass), ClassIndex(0),
	OwnerConstituentInstanceId(InOwnerConstituentInstanceId),
	bUsingPredictedTimestamp(0),
	LifetimeEndTimestamp(0),
	bIsNotCorrected(0),
	bIsDisabledForDestroy(0),
	ServerAwaitClientSyncTimeoutTimestamp(0)
{
	const UCardObject* CardCDO = CardClass.GetDefaultObject();

	//Get the deterministic index of the class.
	if (Class)
	{
		for (uint16 i = 0; i < UFormCoreComponent::GetAllCardObjectClassesSortedByName().Num(); i++)
		{
			if (UFormCoreComponent::GetAllCardObjectClassesSortedByName()[i] == Class->GetClass())
			{
				ClassIndex = i;
				break;
			}
		}
	}
	
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
			LifetimeEndTimestamp = IfPredictedFormCharacter->
				CalculateFuturePredictedTimestamp(CardCDO->DefaultLifetime);
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

FNetCardIdentifier FCard::GetNetCardIdentifier() const
{
	return FNetCardIdentifier(ClassIndex, OwnerConstituentInstanceId);
}

bool FCard::operator==(const FCard& Other) const
{
	return Class == Other.Class && OwnerConstituentInstanceId == Other.OwnerConstituentInstanceId && bUsingPredictedTimestamp == Other.bUsingPredictedTimestamp &&
		LifetimeEndTimestamp == Other.LifetimeEndTimestamp;
}
