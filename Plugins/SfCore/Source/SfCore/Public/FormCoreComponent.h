// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SfUtility.h"
#include "Components/ActorComponent.h"
#include "FormCoreComponent.generated.h"

class UFormResourceComponent;
class UFormStatComponent;
class USfHealthComponent;
class UFormQueryComponent;
class UFormCharacterComponent;
class UConstituent;
class UInventory;

USTRUCT()
struct SFCORE_API FTimestampedTransformSnapshot
{
	GENERATED_BODY()

	FTimestampedTransformSnapshot();
	
	float Timestamp;

	FTransform Transform;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTriggerDelegate);
DECLARE_DYNAMIC_DELEGATE(FTriggerInputDelegate);

/**
 * The FormCoreComponent is responsible for the core logic of a form.
 * Forms should extend APawn or ACharacter. This component must be replicated.
 * The component mainly holds the slotable hierarchy and its functions.
 * This is not designed to work with switching ASfGameMode as team data is cached.
 * Spawn in new forms if the game mode has to be changed.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class SFCORE_API UFormCoreComponent : public UActorComponent
{
	
	GENERATED_BODY()

public:
	UFormCoreComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintPure)
	const TArray<UInventory*>& GetInventories();

	UFUNCTION(BlueprintPure)
	FGameplayTag GetTeam() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Server_SetTeam(const FGameplayTag InTeam);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UInventory* Server_AddInventory(const TSubclassOf<UInventory>& InInventoryClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Server_RemoveInventoryByIndex(const int32 InIndex);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_RemoveInventory(UInventory* Inventory);

	UFUNCTION(BlueprintCallable)
	void Client_SetToFirstPerson();

	UFUNCTION(BlueprintCallable)
	void Client_SetToThirdPerson();
	
	void InternalClientSetToFirstPerson();
	
	void InternalClientSetToThirdPerson();

	UFUNCTION(BlueprintPure)
	bool IsFirstPerson();

	static const TArray<UClass*>& GetAllCardObjectClassesSortedByName();

	UFormQueryComponent* GetFormQuery() const;

	USfHealthComponent* GetHealth() const;

	UFormStatComponent* GetFormStat() const;

	UFormResourceComponent* GetFormResource() const;
	
	void SetMovementStatsDirty() const;

	//False if trigger doesn't exist.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_ActivateTrigger(FGameplayTag Trigger);

	//False if trigger doesn't exist.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_BindTrigger(FGameplayTag Trigger, FTriggerInputDelegate EventToBind);

	UFUNCTION(BlueprintPure, BlueprintAuthorityOnly)
	bool Server_HasTrigger(FGameplayTag Trigger);

	//For lag compensated hit registration.
	void ServerRollbackLocation(const float ServerTimestamp);

	//Must be called after ServerRollbackLocation to bring animation back to current time.
	void ServerRestoreLatestLocation() const;

	//References to all the constituents that isn't ordered. Used to iterate through all owned constituents on a form
	//without accessing intermediate inventories and slotables.
	UPROPERTY(Replicated)
	TArray<UConstituent*> ConstituentRegistry;

	UPROPERTY(EditAnywhere, Category = "FormCoreComponent")
	TArray<FGameplayTag> TriggersToUse;

	//Ticks the low frequency tick event on constituents at this rate.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FormCoreComponent", meta = (ClampMin = 1, ClampMax = 20))
	int32 LowFrequencyTicksPerSecond = 10;

	float CalculatedTimeBetweenLowFrequencyTicks = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere, Category = "FormCoreComponent")
	float WalkSpeedStat;

	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere, Category = "FormCoreComponent")
	float SwimSpeedStat;

	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere, Category = "FormCoreComponent")
	float FlySpeedStat;

	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere, Category = "FormCoreComponent")
	float AccelerationStat;

	UPROPERTY(Replicated, BlueprintReadOnly)
	UFormCharacterComponent* FormCharacter;

	UPROPERTY(Replicated, BlueprintReadOnly)
	UFormQueryComponent* FormQuery;

	UPROPERTY(Replicated, BlueprintReadOnly)
	USfHealthComponent* SfHealth;

	UPROPERTY(Replicated, BlueprintReadOnly)
	UFormStatComponent* FormStat;

	UPROPERTY(Replicated, BlueprintReadOnly)
	UFormResourceComponent* FormResource;

	//Must also be set in DefaultEngine.ini
	UPROPERTY(EditDefaultsOnly)
	uint32 ServerTickRate = 50;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//Inventories to create when this form is spawned.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FormCoreComponent")
	TArray<TSubclassOf<UInventory>> DefaultInventoryClasses;

private:
	
	UFUNCTION()
	void OnRep_Inventories();
	
	UPROPERTY(Replicated, VisibleAnywhere, ReplicatedUsing = OnRep_Inventories, Category = "FormCoreComponent")
	TArray<UInventory*> Inventories;

	UPROPERTY()
	TSet<UInventory*> ClientSubObjectListRegisteredInventories;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "FormCoreComponent")
	FGameplayTag Team;

	UPROPERTY(VisibleAnywhere, Category = "FormCoreComponent")
	bool bIsFirstPerson = false;

	//CardObjectClasses are indexed as they're net referenced very often and is worth using a simplified index for instead
	//of repeatedly serializing the class reference.
	inline static TArray<UClass*> AllCardObjectClassesSortedByName = TArray<UClass*>();

	inline static bool CardObjectClassesFetched = false;
	
	TMap<FGameplayTag, FTriggerDelegate> Triggers;
	
	float LowFrequencyTickDeltaTime = 0;

	bool bInputsRequireSetup = true;

	float TimeSinceLastSnapshot = 0;

	uint8 IndexOfOldestSnapshot = 0;

	TArray<FTimestampedTransformSnapshot> ServerLocationSnapshots;

	FTransform CurrentTransform;

	float TimeBetweenServerLocationSnapshots;

	FSfTickFunctionHelper TenSecondTickHelper{this, &UFormCoreComponent::TenSecondTick, 10.f};

	void TenSecondTick(const float DeltaTime);
};
