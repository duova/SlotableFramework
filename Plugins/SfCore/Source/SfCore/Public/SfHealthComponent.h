// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Constituent.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "SfHealthComponent.generated.h"


class UFormStatComponent;
class USfHealthComponent;
class UFormCoreComponent;
class UConstituent;
class UHealthChangeProcessor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHealthChange, const float&, InHealthBefore, float&, MutableHealthAfter, UConstituent*, Source, const TArray<TSubclassOf<UHealthChangeProcessor>>&, InProcessors);

//Data that represents damage done to the health by a certain source using a certain health change processor.
USTRUCT(BlueprintType)
struct SFCORE_API FHealthChangeData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	float InValue; //Before processing.

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	float OutValue; //Final change to health.

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	UConstituent* Source; //Source of change.

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TArray<TSubclassOf<UHealthChangeProcessor>> Processors; //Processors used.

	float TimeoutTimestamp; //Timestamp of when this data becomes irrelevant.

	FHealthChangeData();

	FHealthChangeData(const float InValue, const float OutValue, UConstituent* Source, const TArray<TSubclassOf<UHealthChangeProcessor>>& InProcessors, const float InTimeoutTimestamp);

	bool operator==(const FHealthChangeData& Other) const;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FHealthChangeData> : TStructOpsTypeTraitsBase2<FHealthChangeData>
{
	enum
	{
		WithNetSerializer = true,
		WithIdenticalViaEquality = true,
		WithCopy = true
	};
};

//HealthChangeProcessor is essentially a definition of a type of "damage" dealt.
//It's called health change because both healing and damage uses a processor.
//These run in serial from lowest index to highest index.
//Convention is that we end damage class names with DamageProcessor, and heal class names with HealProcessor.
//For example a "LifestealDamageProcessor" might heal the owner of the source with the damage dealt, while
//"SpecialTypeDamageProcessor" might check for certain variables on stats to modify the damage dealt.
UCLASS(Blueprintable)
class SFCORE_API UHealthChangeProcessor : public UObject
{
	GENERATED_BODY()

public:
	UHealthChangeProcessor();

	//Out value is applied directly to health.
	//Damage classes should take positive value and return a negative value, heal classes should take a positive value and
	//return a positive value.
	//Both the source and target can be a nullptr, so a validation check must be made.
	UFUNCTION(BlueprintImplementableEvent)
	void ProcessHealthChange(const float InValue, UConstituent* Source, USfHealthComponent* Target, float& OutValue);
};

//Death handlers are the implementations of what happens when the actor of a health component dies.
//This gives an array of OrderedRecentHealthChange which can be used to indicate the constituents that caused death,
//the types of health changes done, and the amount of damage done by each source.
UCLASS(Blueprintable)
class SFCORE_API UDeathHandler : public UObject
{
	GENERATED_BODY()
	
public:
	UDeathHandler();

	void InternalServerOnDeath(USfHealthComponent* Health, const TArray<FHealthChangeData>& InOrderedRecentHealthChange);

	//For OrderedRecentHealthChange, lower index is more recent.
	UFUNCTION(BlueprintImplementableEvent)
	void Server_OnDeath(USfHealthComponent* Health, const TArray<FHealthChangeData>& InOrderedRecentHealthChange);

	UFUNCTION(Client, Reliable)
	virtual void InternalClientOnDeathRPC(USfHealthComponent* Health, const TArray<FHealthChangeData>& InOrderedRecentHealthChange);

	//For OrderedRecentHealthChange, lower index is more recent.
	UFUNCTION(BlueprintImplementableEvent)
	void Autonomous_OnDeath(USfHealthComponent* Health, const TArray<FHealthChangeData>& InOrderedRecentHealthChange);
};

//Health component that can be used with a form, but can also be used by itself on an actor.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SFCORE_API USfHealthComponent : public UActorComponent
{
	GENERATED_BODY()

	friend struct FHealthChangeData;

public:
	USfHealthComponent();

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//Arrays in these functions are intentionally left as copy value as in BP the user is expected to create processor class
	//references in the function call.
	
	//Returns final change to health.
	//Try to make sure damage is done by a constituent, if that is not possible nullptr is not recommended but allowed.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	virtual float ApplyHealthChange(const float InValue, UConstituent* Source, const TArray<TSubclassOf<UHealthChangeProcessor>>& InProcessors);
	
	//Returns final change to health.
	//Try to make sure damage is done by a constituent, if that is not possible nullptr is not recommended but allowed.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	virtual float ApplyHealthChangeFractionOfMax(const float InValue, UConstituent* Source, const TArray<TSubclassOf<UHealthChangeProcessor>>& InProcessors);

	//Returns final change to health.
	//Try to make sure damage is done by a constituent, if that is not possible nullptr is not recommended but allowed.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	virtual float ApplyHealthChangeFractionOfRemaining(const float InValue, UConstituent* Source, const TArray<TSubclassOf<UHealthChangeProcessor>>& InProcessors);

	UFUNCTION(BlueprintPure)
	virtual float GetHealth() const;

	UFUNCTION(BlueprintPure)
	virtual float GetMaxHealth();

	UFUNCTION(BlueprintPure)
	UFormCoreComponent* GetFormCore() const;

	void SetupSfHealth(UFormCoreComponent* InFormCore);
	
	void SecondarySetupSfHealth();
	
	virtual const void AddHealthChangeDataAndCompress(const float InValue, const float OutValue, UConstituent* Source,
									   const TArray<TSubclassOf<UHealthChangeProcessor>>& InProcessors,
									   const float InTimeoutTimestamp);

	virtual void TrimTimeout();

	UPROPERTY(BlueprintAssignable)
	FOnHealthChange Server_OnHealthChange;

protected:
	UPROPERTY(Replicated)
	float Health;

	//Stat to use for max health if stats are available.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SfHealthComponent")
	FGameplayTag MaxHealthStat;

	//Stat to use for constant health regeneration if stats are available.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SfHealthComponent")
	FGameplayTag HealthRegenerationPerSecondStat;

	//Stat to use for constant health degeneration if stats are available.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SfHealthComponent")
	FGameplayTag HealthDegenerationPerSecondStat;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SfHealthComponent", meta = (ClampMin = 1, ClampMax = 10))
	int32 HealthConstantUpdatesPerSecond = 2;

	float CalculatedTimeBetweenHealthUpdates = 0;

	float HealthUpdateTimer = 0;

	//Used if stat doesn't exist or if the component is not on a form.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (ClampMin = 0.1, ClampMax = 999999), Category = "SfHealthComponent")
	float MaxHealthFallback = 0.1;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SfHealthComponent")
	TSubclassOf<UDeathHandler> DeathHandlerClass;

	//Time before a recent health change is considered irrelevant.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (ClampMin = 0.5, ClampMax = 40), Category = "SfHealthComponent")
	float RecentHealthChangeDataTimeout = 15;

	//Lower index is more recent.
	TArray<FHealthChangeData> OrderedRecentHealthChange;
	
	UPROPERTY()
	UFormCoreComponent* FormCore;

	UPROPERTY()
	UFormStatComponent* FormStat;

private:
	inline static TArray<UClass*> AllHealthChangeProcessorClassesSortedByName = TArray<UClass*>();

	inline static bool HealthChangeProcessorClassesFetched = false;
};
