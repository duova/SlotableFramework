// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "FormCoreComponent.generated.h"

class UFormStatComponent;
class USfHealthComponent;
class UFormQueryComponent;
class UFormCharacterComponent;
class UConstituent;
class UInventory;

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

	/**
	 * Read-only copy of inventories.
	 */
	UFUNCTION(BlueprintPure)
	TArray<UInventory*> GetInventories();

	UFUNCTION(BlueprintPure)
	FGameplayTag GetTeam();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Server_SetTeam(const FGameplayTag InTeam);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UInventory* Server_AddInventory(const TSubclassOf<UInventory>& InventoryClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Server_RemoveInventoryByIndex(const int32 Index);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_RemoveInventory(UInventory* Inventory);
	
	void ClientSetToFirstPerson();
	
	void ClientSetToThirdPerson();

	UFUNCTION(BlueprintPure)
	bool IsFirstPerson();
	
	UPROPERTY(Replicated)
	float NonCompensatedServerWorldTime;

	//Works for client and server.
	float GetNonCompensatedServerWorldTime() const;

	//Works for client and server.
	float CalculateFutureServerTimestamp(const float AdditionalTime) const;

	//Works for client and server.
	float CalculateTimeUntilServerTimestamp(const float Timestamp) const;

	//Works for client and server.
	float CalculateTimeSinceServerTimestamp(const float Timestamp) const;

	//Works for client and server.
	bool HasServerTimestampPassed(const float Timestamp) const;

	static const TArray<UClass*>& GetAllCardObjectClassesSortedByName();

	UFormQueryComponent* GetFormQuery() const;

	USfHealthComponent* GetHealth() const;

	UFormStatComponent* GetFormStat() const;

	UPROPERTY()
	TArray<UConstituent*> ConstituentRegistry;

protected:
	virtual void BeginPlay() override;

	virtual void BeginDestroy() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<UInventory>> DefaultInventoryClasses;

private:
	UPROPERTY(Replicated, VisibleAnywhere)
	TArray<UInventory*> Inventories;

	UPROPERTY(Replicated, VisibleAnywhere)
	FGameplayTag Team;

	UPROPERTY(VisibleAnywhere)
	uint8 bIsFirstPerson:1;

	inline static TArray<UClass*> AllCardObjectClassesSortedByName = TArray<UClass*>();

	inline static bool CardObjectClassesFetched = false;

	UPROPERTY()
	UFormCharacterComponent* FormCharacter;

	UPROPERTY()
	UFormQueryComponent* FormQuery;

	UPROPERTY()
	USfHealthComponent* SfHealth;

	UPROPERTY()
	UFormStatComponent* FormStat;
};
