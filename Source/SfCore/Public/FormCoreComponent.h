// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "FormCoreComponent.generated.h"

/**
 * The FormCoreComponent is responsible for the core logic of a form.
 * Forms should extend APawn or ACharacter. This component must be replicated.
 * The component mainly holds the slotable hierarchy and its functions.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SFCORE_API UFormCoreComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFormCoreComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintGetter)
	TArray<UInventory*> GetInventories();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UInventory* AddInventory(const TSubclassOf<UInventory>& InventoryClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RemoveInventoryByIndex(const int32 Index);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool RemoveInventory(UInventory* Inventory);

protected:
	virtual void BeginPlay() override;

	virtual void BeginDestroy() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<UInventory>> DefaultInventoryClasses;

private:
	UPROPERTY(Replicated, VisibleAnywhere)
	TArray<UInventory*> Inventories;
};
