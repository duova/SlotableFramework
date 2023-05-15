// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory.h"
#include "Card.generated.h"

/**
 * A card slotable, or just card, is a lightweight slotable that can only have one instance per inventory.
 * Cards are blueprintable themselves, but they don't have constituents like slotables.
 * They are lightweight because they only exist as a blueprintable UObject on the client.
 * On the server, only their metadata is kept, so no functionality can be performed.
 * The purpose of cards is to create markers that should be represented on the client but do not actually do anything
 * themselves.
 * Card changes can be fully predicted due to their simplicity, unlike normal slotables.
 * Card implementations should be written with the assumption that they can be deinit and destroyed at any given moment.
 */
UCLASS(Blueprintable)
class SFCORE_API UCard : public UObject
{
	GENERATED_BODY()

public:
	UCard();
	
	virtual void BeginDestroy() override;

	UPROPERTY(BlueprintReadOnly)
	UInventory* OwningInventory;

	void Initialize();

	void Deinitialize();
};
