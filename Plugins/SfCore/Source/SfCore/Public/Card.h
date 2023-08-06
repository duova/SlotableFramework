// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FormCoreComponent.h"
#include "Card.generated.h"

class UFormCharacterComponent;
class UCardObject;

/**
 * A card is a marker stored in an inventory. Their addition and removal is fully predicted and they have predicted
 * lifetimes (which are intended to be used as predicted timers). Each card is created by a card object, a blueprintable
 * object that can be spawned on the client inventory when they have the card. Card objects also contain certain default
 * properties of a card and their class is used as an identifier. A card can be shared or owned. Owned cards can only be
 * added by their owner constituent but removed by anyone. These are destroyed automatically when their owner is destroyed.
 * It is possible to search for the card by owner and class in each inventory, and as such only one card of a class and
 * owner may exist per inventory. Shared cards have no owner and only one of each class can exist in each inventory.
 */
USTRUCT(BlueprintType)
struct SFCORE_API FCard
{
	GENERATED_BODY()
	
	enum class ECardType
	{
		DoNotUseLifetime,
		UseDefaultLifetimePredictedTimestamp,
		UseDefaultLifetimeServerTimestamp,
		UseCustomLifetimePredictedTimestamp,
		UseCustomLifetimeServerTimestamp
	};

	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<UCardObject> Class;

	//For serialization only.
	uint16 ClassIndex;
	
	uint8 OwnerConstituentInstanceId;
	
	bool bUsingPredictedTimestamp;

	//If negative we don't check.
	float LifetimeEndTimestamp;

	//For predicted forms, this is set to true after card creation on server until either the client syncs up or timeout.
	//We don't check this card while this is true to prevent unnecessary rollbacks.
	uint8 bIsNotCorrected:1;

	//For predicted forms, this is set to true after card should be destroyed on the server until either the client syncs
	//up or timeout. We don't check this card while this is true to prevent unnecessary rollbacks.
	bool bIsDisabledForDestroy;
	
	double ServerAwaitClientSyncTimeoutTimestamp;

	//Time before the server forces a correction if a client fails to register a card change sent by the server through
	//the FormCharacterComponent.
	inline static constexpr float ServerAwaitClientSyncTimeoutDuration = 0.4;

	FCard();
	
	FCard(const TSubclassOf<UCardObject>& InCardClass, const ECardType InCardType, uint8 InOwnerConstituentInstanceId = 0,
	      const UFormCharacterComponent* InNullUnlessUsingPredictedTimestampFormCharacter = nullptr,
	      const UFormCoreComponent* InNullUnlessUsingServerTimestampFormCore = nullptr, const float InCustomLifetime = 0);

	struct FNetCardIdentifier GetNetCardIdentifier() const;

	bool operator==(const FCard& Other) const;

	friend FArchive& operator<<(FArchive& Ar, FCard& Card)
	{
		//We have a list of all CardObject classes that is sorted by name deterministically so we only have to send the index.
		//We try to not send the full index if we don't have to.
		bool bClassIndexIsLarge = Card.ClassIndex > 255;
		Ar.SerializeBits(&bClassIndexIsLarge, 1);
		if (bClassIndexIsLarge)
		{
			Ar << Card.ClassIndex;
		}
		else
		{
			uint8 Value = Card.ClassIndex;
			Ar << Value;
			Card.ClassIndex = Value;
		}
		if (Ar.IsLoading())
		{
			Card.Class = UFormCoreComponent::GetAllCardObjectClassesSortedByName()[Card.ClassIndex];
		}
		Ar << Card.OwnerConstituentInstanceId;
		Ar.SerializeBits(&Card.bIsDisabledForDestroy, 1);
		//Don't serialize anything else if card is disabled, we don't need it.
		if (Card.bIsDisabledForDestroy) return Ar;
		Ar.SerializeBits(&Card.bUsingPredictedTimestamp, 1);
		Ar << Card.LifetimeEndTimestamp;
		return Ar;
	}
};