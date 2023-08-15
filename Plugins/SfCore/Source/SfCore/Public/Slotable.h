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

	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere, Category = "Slotable")
	class UInventory* OwningInventory;
	
	UFUNCTION(BlueprintPure)
	const TArray<UConstituent*>& GetConstituents() const;
	
	UFUNCTION(BlueprintPure)
	TArray<UConstituent*> GetConstituentsOfClass(const TSubclassOf<UConstituent>& InConstituentClass);

	void AutonomousInitialize();

	void ServerInitialize();
	
	void AutonomousDeinitialize();

	void ServerDeinitialize();

	UFUNCTION(BlueprintImplementableEvent)
	void Autonomous_Initialize();

	UFUNCTION(BlueprintImplementableEvent)
	void Server_Initialize();

	UFUNCTION(BlueprintImplementableEvent)
	void Autonomous_Deinitialize();

	UFUNCTION(BlueprintImplementableEvent)
	void Server_Deinitialize();

	UFUNCTION(Client, Reliable)
	void ClientAutonomousInitialize(UInventory* InOwningInventory);

	UFUNCTION(Client, Reliable)
	void ClientAutonomousDeinitialize();

	void AssignConstituentInstanceId(UConstituent* Constituent);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slotable")
	TArray<TSubclassOf<UConstituent>> InitialConstituentClasses;

protected:

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
	
	UPROPERTY(Replicated, VisibleAnywhere, ReplicatedUsing = OnRep_Constituents, Category = "Slotable")
	TArray<UConstituent*> Constituents;

	UPROPERTY()
	TArray<UConstituent*> ClientSubObjectListRegisteredConstituents;

	UConstituent* CreateUninitializedConstituent(const TSubclassOf<UConstituent>& InConstituentClass) const;
};
