// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfObject.h"
#include "Slotable.generated.h"

/**
 * An object in an inventory.
 * The framework allows these to take "passive" or "active" actions.
 * Passive actions hook into events, while active actions are triggered by inputs.
 * In theory this can represent anything from an in-game item to a status effect to an ability.
 * A slotable is itself only supposed to be a container for constituents,
 * which composes a slotables functionality.
 */
UCLASS()
class SFCORE_API USlotable : public USfObject
{
	GENERATED_BODY()

public:

	USlotable();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere)
	UInventory* OwningInventory;
	
	UFUNCTION(BlueprintGetter)
	TArray<UConstituent*> GetConstituents();

	void Initialize();

	void Deinitialize();

protected:

	virtual void BeginDestroy() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<UConstituent>> InitialConstituentClasses;

	/*
	 * Called during slotable init.
	 */
	void InitializeConstituent(UConstituent* Constituent);

	/*
	 * Called during slotable deinit.
	 */
	void DeinitializeConstituent(UConstituent* Constituent);

private:
	UPROPERTY(Replicated, VisibleAnywhere)
	TArray<UConstituent*> Constituents;

	UConstituent* CreateUninitializedConstituent(const TSubclassOf<UConstituent>& ConstituentClass) const;
};
