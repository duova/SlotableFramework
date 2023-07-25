// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SfTradeComponent.generated.h"


struct FGameplayTag;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class SFECONOMY_API USfTradeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USfTradeComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	uint8 bHasInfiniteResources:1;

private:

	UPROPERTY(Replicated)
	TMap<FGameplayTag, uint32> HeldCurrencyValues;
};
