// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfObject.h"
#include "Slotable.generated.h"

class UConstituent;

/**
 * An object in an inventory.
 * The framework allows these to take "passive" or "active" actions.
 * Passive actions hook into events, while active actions are triggered by inputs.
 * In theory this can represent anything from an in-game item to a status effect to an ability.
 * A slotable is itself only supposed to be a container for constituents,
 * which composes a slotable's functionality.
 */
UCLASS(Blueprintable)
class SFCORE_API USlotable : public USfObject
{
	GENERATED_BODY()

public:

	USlotable();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_OwningInventory, Replicated, BlueprintReadOnly, VisibleAnywhere)
	class UInventory* OwningInventory;
	
	UFUNCTION(BlueprintPure)
	const TArray<UConstituent*>& GetConstituents();
	
	UFUNCTION(BlueprintPure)
	TArray<UConstituent*> GetConstituentsOfClass(const TSubclassOf<UConstituent> ConstituentClass);

	void ClientInitialize();

	void ServerInitialize();
	
	void ClientDeinitialize();

	void ServerDeinitialize();

	UFUNCTION(BlueprintImplementableEvent)
	void Client_Initialize();

	UFUNCTION(BlueprintImplementableEvent)
	void Server_Initialize();

	UFUNCTION(BlueprintImplementableEvent)
	void Client_Deinitialize();

	UFUNCTION(BlueprintImplementableEvent)
	void Server_Deinitialize();

	void AssignConstituentInstanceId(UConstituent* Constituent);

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

	UFUNCTION()
	void OnRep_Constituents();
	
	UPROPERTY(Replicated, VisibleAnywhere, ReplicatedUsing = OnRep_Constituents)
	TArray<UConstituent*> Constituents;

	TArray<UConstituent*> ClientSubObjectListRegisteredConstituents;

	UConstituent* CreateUninitializedConstituent(const TSubclassOf<UConstituent>& ConstituentClass) const;

	uint8 bAwaitingClientInit:1;

	UFUNCTION()
	void OnRep_OwningInventory();
};
