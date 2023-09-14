// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "FormResourceComponent.generated.h"

class UFormStatComponent;
class UFormCoreComponent;
//Data about a resource that is available.
USTRUCT(BlueprintType)
struct SFCORE_API FResource
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0, ClampMax = 999999))
	float Value;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag IncreasePerSecondStat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag DecreasePerSecondStat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag MaxValueStat;

	//A value of zero will use the MaxValueStat.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0, ClampMax = 999999))
	float MaxValueOverride;

	FResource();

	bool operator==(const FResource& Other) const;

	bool operator!=(const FResource& Other) const;

	friend FArchive& operator<<(FArchive& Ar, FResource& Resource)
	{
		//We don't serialize the tag because the index is always the same since we can't add a resource during runtime.
		Ar << Resource.Value;
		return Ar;
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FResource> : public TStructOpsTypeTraitsBase2<FResource>
{
	enum
	{
		WithNetSerializer = true,
		WithIdenticalViaEquality = true,
		WithCopy = true
	};
};

/**
 * Holds values for resources.
 * Resources are predicted.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class SFCORE_API UFormResourceComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class UFormCharacterComponent;

public:
	UFormResourceComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	void SetupFormResource(UFormCoreComponent* InFormCore);

	void SecondarySetupFormResource();

	UPROPERTY(EditAnywhere, Category = "FormResourceComponent")
	TArray<FResource> ResourcesToRegister;

	UFUNCTION(BlueprintPure)
	float GetResourceValue(const FGameplayTag InTag) const;
	
	FResource* GetResourceFromTag(const FGameplayTag& InTag);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_AddResourceValue(const FGameplayTag InTag, const float InValue);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_RemoveResourceValue(const FGameplayTag InTag, const float InValue);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_SetResourceValue(const FGameplayTag InTag, const float InValue);

	UFUNCTION(BlueprintCallable)
	bool Predicted_AddResourceValue(const FGameplayTag InTag, const float InValue);

	UFUNCTION(BlueprintCallable)
	bool Predicted_RemoveResourceValue(const FGameplayTag InTag, const float InValue);

	UFUNCTION(BlueprintCallable)
	bool Predicted_SetResourceValue(const FGameplayTag InTag, const float InValue);
	
	float GetMaxValue(const FResource& Resource) const;

	void LocalInternalAddResourceValue(FResource& Resource, const float InValue) const;

	void LocalInternalRemoveResourceValue(FResource& Resource, const float InValue) const;

	void LocalInternalSetResourceValue(FResource& Resource, const float InValue) const;

	//How often resource values are updated for servers and owners.
	UPROPERTY(EditAnywhere, meta = (ClampMin = 0, ClampMax = 10), Category = "FormResourceComponent")
	int32 OwnerResourceUpdateFrequencyPerSecond = 2;

	float CalculatedTimeToEachResourceUpdate = 0;

	//How often resource values are replicated to non-owners.
	UPROPERTY(EditAnywhere, meta = (ClampMin = 0, ClampMax = 10), Category = "FormResourceComponent")
	int32 NonOwnerResourceReplicationFrequencyPerSecond = 1;

	float CalculatedTimeToEachReplication = 0;
	
private:
	UPROPERTY()
	UFormCoreComponent* FormCore;

	UPROPERTY()
	UFormStatComponent* FormStat;

	//Resources are only replicated to non-owners with a lower frequency to reduce bandwidth.
	UPROPERTY(VisibleAnywhere, Replicated)
	TArray<FResource> Resources;

	float NextReplicationServerTimestamp;
};
