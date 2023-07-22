// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Constituent.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "SfHealthComponent.generated.h"


class USfHealthComponent;
class UFormCoreComponent;
class UConstituent;
class UHealthDeltaProcessor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHealthChange, const float&, InHealthBefore, float&, MutableHealthAfter, UConstituent*, Source, const TArray<TSubclassOf<UHealthDeltaProcessor>>&, Processors);

//Data that represents damage done to the health by a certain source using a certain health delta processor.
USTRUCT(BlueprintType)
struct SFCORE_API FHealthDeltaData
{
	GENERATED_BODY()

	float InValue; //Before processing.

	float OutValue; //Final change to health.

	UPROPERTY()
	UConstituent* Source; //Source of change.

	TArray<TSubclassOf<UHealthDeltaProcessor>> Processors; //Processors used.

	float TimeoutTimestamp; //Timestamp of when this data becomes irrelevant.

	FHealthDeltaData();

	FHealthDeltaData(const float InValue, const float OutValue, UConstituent* Source, const TArray<TSubclassOf<UHealthDeltaProcessor>>& Processors, const float TimeoutTimestamp);

	bool operator==(const FHealthDeltaData& Other) const;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FHealthDeltaData> : TStructOpsTypeTraitsBase2<FHealthDeltaData>
{
	enum
	{
		WithNetSerializer = true
	};
};

//HealthDeltaProcessor is essentially a definition of a type of "damage" dealt.
//It's called health delta because both healing and damage uses a processor.
//These run in serial from lowest index to highest index.
//Convention is that we end damage class names with DamageProcessor, and heal class names with HealProcessor.
//For example a "LifestealDamageProcessor" might heal the owner of the source with the damage dealt, while
//"SpecialTypeDamageProcessor" might check for certain variables on stats to modify the damage dealt.
UCLASS(Blueprintable)
class SFCORE_API UHealthDeltaProcessor : public UObject
{
	GENERATED_BODY()

public:
	UHealthDeltaProcessor();

	//Out value is applied directly to health.
	//Damage classes should take positive value and return a negative value, heal classes should take a positive value and
	//return a positive value.
	//Both the source and target can be a nullptr, so a validation check must be made.
	UFUNCTION(BlueprintImplementableEvent)
	void ProcessHealthDelta(const float InValue, UConstituent* Source, USfHealthComponent* Target, float& OutValue);
};

//Death handlers are the implementations of what happens when the actor of a health component dies.
//This gives an array of OrderedRecentHealthDelta which can be used to indicate the constituents that caused death,
//the types of health changes done, and the amount of damage done by each source.
UCLASS(Blueprintable)
class SFCORE_API UDeathHandler : public UObject
{
	GENERATED_BODY()
	
public:
	UDeathHandler();

	void InternalServerOnDeath(USfHealthComponent* Health, const TArray<FHealthDeltaData>& OrderedRecentHealthDelta);

	//For OrderedRecentHealthDelta, lower index is more recent.
	UFUNCTION(BlueprintImplementableEvent)
	void Server_OnDeath(USfHealthComponent* Health, const TArray<FHealthDeltaData>& OrderedRecentHealthDelta);

	UFUNCTION(Client, Reliable)
	virtual void InternalClientOnDeathRPC(USfHealthComponent* Health, const TArray<FHealthDeltaData>& OrderedRecentHealthDelta);

	//For OrderedRecentHealthDelta, lower index is more recent.
	UFUNCTION(BlueprintImplementableEvent)
	void Autonomous_OnDeath(USfHealthComponent* Health, const TArray<FHealthDeltaData>& OrderedRecentHealthDelta);
};

//Health component that can be used with a form, but can also be used by itself on an actor.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SFCORE_API USfHealthComponent : public UActorComponent
{
	GENERATED_BODY()

	friend struct FHealthDeltaData;

public:
	USfHealthComponent();

	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//Arrays in these functions are intentionally left as copy value as in BP the user is expected to create processor class
	//references in the function call.
	
	//Returns final change to health.
	//Try to make sure damage is done by a constituent, if that is not possible nullptr is not recommended but allowed.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	virtual float ApplyHealthDelta(float Value, UConstituent* Source, TArray<TSubclassOf<UHealthDeltaProcessor>> Processors);
	
	//Returns final change to health.
	//Try to make sure damage is done by a constituent, if that is not possible nullptr is not recommended but allowed.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	virtual float ApplyHealthDeltaFractionOfMax(float Value, UConstituent* Source, TArray<TSubclassOf<UHealthDeltaProcessor>> Processors);

	//Returns final change to health.
	//Try to make sure damage is done by a constituent, if that is not possible nullptr is not recommended but allowed.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	virtual float ApplyHealthDeltaFractionOfRemaining(float Value, UConstituent* Source, TArray<TSubclassOf<UHealthDeltaProcessor>> Processors);

	UFUNCTION(BlueprintPure)
	virtual float GetHealth() const;

	UFUNCTION(BlueprintPure)
	virtual float GetMaxHealth();

	UFUNCTION(BlueprintPure)
	UFormCoreComponent* GetFormCore() const;

	void SetupSfHealth(UFormCoreComponent* InFormCore);
	
	virtual const FHealthDeltaData& AddHealthDeltaDataAndCompress(const float InValue, const float OutValue, UConstituent* Source,
									   const TArray<TSubclassOf<UHealthDeltaProcessor>>& Processors,
									   const float TimeoutTimestamp);

	virtual void TrimTimeout();

	UPROPERTY(BlueprintAssignable)
	FOnHealthChange OnHealthChange;

protected:
	UPROPERTY(Replicated)
	float Health;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FGameplayTag MaxHealthStatTag;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float MaxHealthOverride;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TSubclassOf<UDeathHandler> DeathHandlerClass;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (ClampMin = 0.5))
	int32 RecentHealthDeltaDataTimeout;

	//Lower index is more recent.
	TArray<FHealthDeltaData> OrderedRecentHealthDelta;
	
	UPROPERTY()
	UFormCoreComponent* FormCore;

private:
	inline static TArray<UClass*> AllHealthDeltaProcessorClassesSortedByName = TArray<UClass*>();

	inline static bool HealthDeltaProcessorClassesFetched = false;
};
