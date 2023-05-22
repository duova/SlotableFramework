// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FormCoreComponent.h"
#include "Slotable.h"
#include "Card.generated.h"

/**
 * A card is a marker stored in an inventory. Their addition and removal is fully predicted and they have predicted
 * lifetimes (which are intended to be used as predicted timers). Each card is created by a card object, a blueprintable
 * object that can be spawned on the client inventory when they have the card. Card objects also contain certain default
 * properties of a card and their class is used as an identifier. A card can be shared or owned. Owned cards can only be
 * added by their owner constituent but removed by anyone. These are destroyed automatically when their owner is destroyed.
 * It is possible to search for the card by owner and class in each inventory, and as such only one card of a class and
 * owner may exist per inventory. Shared cards have no owner and only one of each class can exist in each inventory.
 */
USTRUCT()
struct FCard
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

	UPROPERTY()
	TSubclassOf<class UCardObject> Class;

	UPROPERTY()
	uint16 OwnerConstituentInstanceId;

	UPROPERTY()
	uint8 bUsingPredictedTimestamp:1;

	//If negative we don't check.
	UPROPERTY()
	float LifetimeEndTimestamp;

	//For predicted forms, this is set to true after card creation on server until either the client syncs up or timeout.
	//We don't check this card while this is true to prevent unnecessary rollbacks.
	UPROPERTY()
	uint8 bIsNotCorrected:1;

	//For predicted forms, this is set to true after card should be destroyed on the server until either the client syncs
	//up or timeout. We don't check this card while this is true to prevent unnecessary rollbacks.
	UPROPERTY()
	uint8 bIsDisabledForDestroy:1;
	
	UPROPERTY()
	float ClientSyncTimeRemaining;

	const float ClientSyncTimeout = 0.4;

	FCard();
	
	FCard(const TSubclassOf<class UCardObject>& CardClass, ECardType CardType, uint16 InOwnerConstituentInstanceId = 0,
	      UFormCharacterComponent* IfPredictedFormCharacter = nullptr,
	      UFormCoreComponent* IfServerFormCore = nullptr, const float CustomLifetime = 0);

	bool operator==(const FCard& Other) const;

	friend FArchive& operator<<(FArchive& Ar, FCard& Card)
	{
		Ar << Card.Class;
		Ar << Card.OwnerConstituentInstanceId;
		Ar.SerializeBits(&Card.bIsDisabledForDestroy, 1);
		//Don't serialize anything else if card is disabled, we don't need it.
		if (Card.bIsDisabledForDestroy) return Ar;
		Ar.SerializeBits(&Card.bUsingPredictedTimestamp, 1);
		Ar << Card.LifetimeEndTimestamp;
		return Ar;
	}
};

/**
 * Card objects are definitions for cards, but may be optionally spawned on the client for the lifetime of the card. They
 * are blueprintable, but should be expected to be initialized or deinitialized at any time.
 */
UCLASS(Blueprintable)
class SFCORE_API UCardObject : public UObject
{
	GENERATED_BODY()

public:
	UCardObject();
	
	virtual void BeginDestroy() override;

	UPROPERTY(BlueprintReadOnly)
	class UInventory* OwningInventory;

	void Initialize();

	void Deinitialize();

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	uint8 bSpawnCardObjectOnClient:1;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	uint8 bUseLifetime:1;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float DefaultLifetime;
	
	uint16 OwnerConstituentInstanceId;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float DeltaMovementSpeed;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float DeltaMultiplicativeMovementSpeed;

	//Multiplies the product instead of adding to the multiplier.
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float SecondDegreeDeltaMultiplicativeMovementSpeed;
};
