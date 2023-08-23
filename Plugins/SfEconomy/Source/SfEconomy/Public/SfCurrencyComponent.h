// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "SfCurrencyComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSfEconomy, Log, All);

//The amount of a certain currency type.
USTRUCT(BlueprintType)
struct SFECONOMY_API FCurrencyValuePair
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FGameplayTag Currency;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	int32 Value;

	FCurrencyValuePair();

	bool operator==(const FCurrencyValuePair& Other) const;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FCurrencyValuePair> : public TStructOpsTypeTraitsBase2<FCurrencyValuePair>
{
	enum
	{
		WithNetSerializer = true,
		WithIdenticalViaEquality = true,
		WithCopy = true
	};
};

//Container for different currencies identified by a FGameplayTag.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SFECONOMY_API USfCurrencyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USfCurrencyComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, BlueprintAuthorityOnly)
	bool Server_HasCurrency(const FGameplayTag InCurrency) const;

	//Returns 0 if currency doesn't exist.
	UFUNCTION(BlueprintPure, BlueprintAuthorityOnly)
	int32 Server_GetCurrencyValue(const FGameplayTag InCurrency) const;

	//Returns false if currency doesn't exist. Adds to a max value of 2,147,483,647, operation will be rejected if not possible or negative value provided.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_AddCurrency(const FGameplayTag InCurrency, const int32 InValue);

	//Returns false if currency doesn't exist. Deducts to a min value of 0. Operation is rejected if value is negative.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_DeductCurrency(const FGameplayTag InCurrency, const int32 InValue);

	//Returns false if currency doesn't exist. Value must be between 0 and 2,147,483,647 or the operation will be rejected.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_SetCurrency(const FGameplayTag InCurrency, const int32 InValue);

private:
	UPROPERTY(Replicated, EditAnywhere)
	TArray<FCurrencyValuePair> HeldCurrencyValues;
};
