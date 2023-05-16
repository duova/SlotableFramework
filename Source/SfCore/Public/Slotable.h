// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfObject.h"
#include "Constituent.h"
#include "Slotable.generated.h"

/**
 * An object in an inventory.
 * The framework allows these to take "passive" or "active" actions.
 * Passive actions hook into events, while active actions are triggered by inputs.
 * In theory this can represent anything from an in-game item to a status effect to an ability.
 * A slotable is itself only supposed to be a container for constituents,
 * which composes a slotable's functionality.
 */
UCLASS()
class SFCORE_API USlotable : public USfObject
{
	GENERATED_BODY()

public:

	USlotable();

	void Tick(float DeltaTime);
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_OwningInventory, Replicated, BlueprintReadOnly, VisibleAnywhere)
	class UInventory* OwningInventory;

	/**
	 * Read-only copy of constituents.
	 */
	UFUNCTION(BlueprintGetter)
	TArray<UConstituent*> GetConstituents();

	void ClientInitialize();

	void ServerInitialize();
	
	void ClientDeinitialize();

	void ServerDeinitialize();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<UConstituent>> InitialConstituentClasses;

protected:

	virtual void BeginDestroy() override;

	/*
	 * Called during slotable init.
	 */
	void ServerInitializeConstituent(UConstituent* Constituent);

	/*
	 * Called during slotable deinit.
	 */
	void ServerDeinitializeConstituent(UConstituent* Constituent);

private:
	UPROPERTY(Replicated, VisibleAnywhere)
	TArray<UConstituent*> Constituents;

	UConstituent* CreateUninitializedConstituent(const TSubclassOf<UConstituent>& ConstituentClass) const;

	uint8 bAwaitingClientInit:1;

	UFUNCTION()
	void OnRep_OwningInventory();
};
