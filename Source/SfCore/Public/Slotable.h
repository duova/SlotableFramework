// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfObject.h"
#include "Slotable.generated.h"

/**
 * A representation of anything in an inventory from an item to an ability.
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
