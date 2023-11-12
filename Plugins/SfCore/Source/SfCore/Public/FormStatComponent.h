// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "FormStatComponent.generated.h"

class UFormStatComponent;
struct FStatArray;

//A tag-float pair that represents a value (which can be a direct value or multiplicative) for a single stat.
USTRUCT(BlueprintType)
struct SFCORE_API FStat : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FGameplayTag StatTag;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0, ClampMax = 999999))
	float Value;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0, ClampMax = 999999))
	float MaxValue = 999999;
	
	FStat();

	FStat(const FGameplayTag& InStatTag, const float InValue);

	bool operator==(const FStat& Other) const;
	
	bool operator!=(const FStat& Other) const;
	
	void PostReplicatedAdd(const FStatArray& InArraySerializer);
	
	void PostReplicatedChange(const FStatArray& InArraySerializer);

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FStat> : TStructOpsTypeTraitsBase2<FStat>
{
	enum 
	{
		WithNetSerializer = true,
		WithIdenticalViaEquality = true,
		WithCopy = true
   };
};

USTRUCT(BlueprintType)
struct FStatArray : public FFastArraySerializer
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FStat> Items;

	UPROPERTY()
	UFormStatComponent* OwningFormStat;

	FStatArray();
	
	bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
	{
		return FastArrayDeltaSerialize<FStat, FStatArray>( Items, DeltaParms, *this );
	}
};

template<>
struct TStructOpsTypeTraits<FStatArray> : TStructOpsTypeTraitsBase2<FStatArray>
{
	enum 
	{
		WithNetDeltaSerializer = true,
		WithCopy = true
   };
};

//Types of stat modifiers in order of application.
UENUM(BlueprintType)
enum EStatModifierType
{
	Additive, //Add the value to base stat value.
	AdditiveMultiplicative, //Sum modifiers together, +1, and multiplies with previous value. 0.1 is +10%, stacking additively.
	TrueMultiplicative, //Multiplies to previous value, multiple modifiers multiply. 1.1 is +10%, which is stacked exponentially.
	FlatAdditive //Adds to value after all other modifications.
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCurrentStatChangeDelegate);

/**
 * Form component for tracking form stats used for constituents.
 * Not to be confused with gameplay statistics/analytics.
 * The modified stat values are clamped between 0 and 999999.
 * 
 * ***Stat calculations in SF***
 *
 * We combine each Stat in this deterministic order, only Stats with the same StatTag will be considered:
 * - Start with value found in BaseStats.
 * - Add values found in AdditiveStatModifiers to base value.
 * - Sum AdditiveMultiplicativeStatModifiers together, +1, and multiply with the previous value.
 * - Multiply by the values in TrueMultiplicativeStatModifiers.
 * - Add values in FlatStatModifiers.
 *
 * The way these stat numbers are uses is up to the user. But the recommendation is that normal stats (eg. max health) uses
 * the values as is while percentage stats (eg. percentage damage taken) is expressed as a decimal (100% = 1), also that
 * percentage reductions should be modifiers to the actual stat as opposed to being the actual stat. (eg. 10% damage reduction
 * is a 0.9 TrueMultiplicativeStatModifier to PercentageDamageTaken as opposed to PercentageDamageReduction being a stat)
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SFCORE_API UFormStatComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UFormStatComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool CalculateStat(const FGameplayTag& InStatTag);

	//The user should clean up stat modifiers after use.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	virtual const FStat& Server_AddStatModifier(const FGameplayTag InStatTag, const EStatModifierType InType, const float InValue);

	//The user should clean up stat modifiers after use.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	virtual TArray<FStat> Server_AddStatModifierBatch(const TArray<FStat> InStats, const EStatModifierType InType);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	virtual bool Server_RemoveStatModifier(const FStat& InModifier, const EStatModifierType InType);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	virtual bool Server_RemoveStatModifierBatch(TArray<FStat> InModifiers, const EStatModifierType InType);

	//Add BP functions to update UI when stats change.
	UPROPERTY(BlueprintAssignable)
	FCurrentStatChangeDelegate OnClientStatsChange;
	
	UFUNCTION(BlueprintPure)
	virtual TArray<FStat>& GetCurrentStats();

	UFUNCTION(BlueprintPure)
	virtual float GetStat(const FGameplayTag StatTag);

	void SetupFormStat();

protected:

	//Only stats that already exist in base stats can be modified.
	//Even if a stat is 0 as a base, they need to be registered in base stats, or any changes to them will be ignored.
	//Current stats will always have the same stats as base stats but just different values.
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<FStat> BaseStats;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TArray<FStat> AdditiveStatModifiers;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TArray<FStat> AdditiveMultiplicativeStatModifiers;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TArray<FStat> TrueMultiplicativeStatModifiers;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TArray<FStat> FlatAdditiveStatModifiers;

	//Current stats will always be positive.
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Replicated)
	FStatArray CurrentStats;
};
