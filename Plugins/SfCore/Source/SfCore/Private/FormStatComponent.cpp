// Fill out your copyright notice in the Description page of Project Settings.


#include "FormStatComponent.h"

#include "SfObject.h"
#include "Net/UnrealNetwork.h"


FStat::FStat(): Value(0)
{
}

FStat::FStat(const FGameplayTag& InStatTag, const float InValue): StatTag(InStatTag), Value(InValue)
{
}

bool FStat::operator==(const FStat& Other) const
{
	if (Other.StatTag == StatTag && Other.Value == Value) return true;
	return false;
}

bool FStat::operator!=(const FStat& Other) const
{
	return !(Other == *this);
}

//We call OnClientStatsChange when anything has changed.
void FStat::PostReplicatedAdd(const FStatArray& InArraySerializer)
{
	if (InArraySerializer.OwningFormStat->GetOwner()->HasAuthority()) return;
	InArraySerializer.OwningFormStat->OnClientStatsChange.Broadcast();
}

void FStat::PostReplicatedChange(const FStatArray& InArraySerializer)
{
	if (InArraySerializer.OwningFormStat->GetOwner()->HasAuthority()) return;
	InArraySerializer.OwningFormStat->OnClientStatsChange.Broadcast();
}

bool FStat::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	StatTag.NetSerialize(Ar, Map, bOutSuccess);
	Ar << Value;
	return bOutSuccess;
}

FStatArray::FStatArray(): OwningFormStat(nullptr)
{
}

UFormStatComponent::UFormStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CurrentStats.OwningFormStat = this;
	SetIsReplicatedByDefault(true);
}

void UFormStatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS(UFormStatComponent, CurrentStats, DefaultParams);
}

bool UFormStatComponent::CalculateStat(const FGameplayTag& InStatTag)
{
	const FStat* BaseStat = BaseStats.FindByPredicate([InStatTag](const FStat& Stat){ return Stat.StatTag == InStatTag;});
	FStat* CurrentStat = CurrentStats.Items.FindByPredicate([InStatTag](const FStat& Stat)
	{
		return Stat.StatTag == InStatTag;
	});
	//We don't need to calculate if stat doesn't exist in base or current stats.
	if (!BaseStat || !CurrentStat) return false;

	//We pass the value through all the modifiers in the order specified in the header file.
	float Value = BaseStat->Value;
	for (const FStat& AdditiveStat : AdditiveStatModifiers)
	{
		if (AdditiveStat.StatTag == InStatTag)
		{
			Value = FMath::Clamp(Value + AdditiveStat.Value, 0, BaseStat->MaxValue);
		}
	}
	float ToMultiply = 1.f;
	for (const FStat& AdditiveMultiplicativeStat : AdditiveMultiplicativeStatModifiers)
	{
		if (AdditiveMultiplicativeStat.StatTag == InStatTag)
		{
			ToMultiply = FMath::Clamp(ToMultiply + AdditiveMultiplicativeStat.Value, 0, BaseStat->MaxValue);
		}
	}
	Value = FMath::Clamp(Value * ToMultiply, 0, BaseStat->MaxValue);
	for (const FStat& TrueMultiplicativeStat : TrueMultiplicativeStatModifiers)
	{
		if (TrueMultiplicativeStat.StatTag == InStatTag)
		{
			Value = FMath::Clamp(Value * TrueMultiplicativeStat.Value, 0, BaseStat->MaxValue);
		}
	}
	for (const FStat& FlatAdditiveStat : FlatAdditiveStatModifiers)
	{
		if (FlatAdditiveStat.StatTag == InStatTag)
		{
			Value = FMath::Clamp(Value + FlatAdditiveStat.Value, 0, BaseStat->MaxValue);
		}
	}
	CurrentStat->Value = Value;
	CurrentStats.MarkItemDirty(*CurrentStat);
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormStatComponent, CurrentStats, this);
	return true;
}

const FStat& UFormStatComponent::Server_AddStatModifier(const FGameplayTag InStatTag, const EStatModifierType InType,
	const float InValue)
{
	const FStat* StatInstance = nullptr;
	switch (InType)
	{
	case Additive:
		StatInstance = &AdditiveStatModifiers[AdditiveStatModifiers.Emplace(InStatTag, InValue)];
		break;
	case AdditiveMultiplicative:
		StatInstance = &AdditiveMultiplicativeStatModifiers[AdditiveMultiplicativeStatModifiers.Emplace(InStatTag, InValue)];
		break;
	case TrueMultiplicative:
		StatInstance = &TrueMultiplicativeStatModifiers[TrueMultiplicativeStatModifiers.Emplace(InStatTag, InValue)];
		break;
	case FlatAdditive:
		StatInstance = &FlatAdditiveStatModifiers[FlatAdditiveStatModifiers.Emplace(InStatTag, InValue)];
		break;
	default:
		UE_LOG(LogTemp, Error, TEXT("Server_AddStatModifier was called with an invalid EStatModifierType."));
		break;
	}

	//This updates the relevant stat and sends updates to clients.
	CalculateStat(InStatTag);
	checkf(StatInstance, TEXT("StatInstance could not be created."));
	
	//The stat instance is returned so it can be removed.
	return *StatInstance;
}

TArray<FStat> UFormStatComponent::Server_AddStatModifierBatch(const TArray<FStat> InStats, const EStatModifierType InType)
{
	TArray<FStat> ReturnStats;
	switch (InType)
	{
	case Additive:
		AdditiveStatModifiers.Reserve(InStats.Num());
		break;
	case AdditiveMultiplicative:
		AdditiveMultiplicativeStatModifiers.Reserve(InStats.Num());
		break;
	case TrueMultiplicative:
		TrueMultiplicativeStatModifiers.Reserve(InStats.Num());
		break;
	case FlatAdditive:
		FlatAdditiveStatModifiers.Reserve(InStats.Num());
		break;
	}
	ReturnStats.Reserve(InStats.Num());
	for (const FStat& Stat : InStats)
	{
		ReturnStats.Add(Server_AddStatModifier(Stat.StatTag, InType, Stat.Value));
	}

	//The stat instance is returned so it can be removed.
	return ReturnStats;
}

bool UFormStatComponent::Server_RemoveStatModifier(const FStat& InModifier, const EStatModifierType InType)
{
	bool bRemoved = false;
	switch (InType)
	{
	case Additive:
		bRemoved = AdditiveStatModifiers.RemoveSingle(InModifier) != 0;
		break;
	case AdditiveMultiplicative:
		bRemoved = AdditiveMultiplicativeStatModifiers.RemoveSingle(InModifier) != 0;
		break;
	case TrueMultiplicative:
		bRemoved = TrueMultiplicativeStatModifiers.RemoveSingle(InModifier) != 0;
		break;
	case FlatAdditive:
		bRemoved = FlatAdditiveStatModifiers.RemoveSingle(InModifier) != 0;
		break;
	default:
		return false;
	}
	
	//This updates the relevant stat.
	CalculateStat(InModifier.StatTag);
	return bRemoved;
}

bool UFormStatComponent::Server_RemoveStatModifierBatch(TArray<FStat> InModifiers, const EStatModifierType InType)
{
	bool bRemovedSomething = false;
	TArray<FStat>* StatModifierArray = nullptr;
	switch (InType)
	{
	case Additive:
		StatModifierArray = &AdditiveStatModifiers;
		break;
	case AdditiveMultiplicative:
		StatModifierArray = &AdditiveMultiplicativeStatModifiers;
		break;
	case TrueMultiplicative:
		StatModifierArray = &TrueMultiplicativeStatModifiers;
		break;
	case FlatAdditive:
		StatModifierArray = &FlatAdditiveStatModifiers;
		break;
	default:
		return false;
	}
	for (int16 i = StatModifierArray->Num(); i >= 0; i--)
	{
		const FStat& StatModifier = (*StatModifierArray)[i];
		const int16 InModifierIndex = InModifiers.Find(StatModifier);
		if (InModifierIndex == INDEX_NONE) continue;
		InModifiers.RemoveAtSwap(InModifierIndex, 1, false);
		StatModifierArray->RemoveAt(i, 1, false);
		bRemovedSomething = true;
		CalculateStat(StatModifier.StatTag);
	}
	StatModifierArray->Shrink();
	return bRemovedSomething;
}

TArray<FStat>& UFormStatComponent::GetCurrentStats()
{
	return CurrentStats.Items;
}

float UFormStatComponent::GetStat(const FGameplayTag StatTag)
{
	const FStat* CurrentStat = CurrentStats.Items.FindByPredicate([StatTag](const FStat& Stat)
	{
		return Stat.StatTag == StatTag;
	});
	if (!CurrentStat) return 0;
	return CurrentStat->Value;
}

void UFormStatComponent::SetupFormStat()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	for (const FStat& Stat : BaseStats)
	{
		if (!Stat.StatTag.IsValid())
		{
			UE_LOG(LogSfCore, Error, TEXT("Found invalid StatTag in BaseStats in UFormStatComponent."));
		}
	}
	if (TArrayCheckDuplicate(BaseStats, [](const FStat& CheckedStatA, const FStat& CheckedStatB)
	{
		return CheckedStatA.StatTag == CheckedStatB.StatTag;
	}))
	{
		UE_LOG(LogSfCore, Error, TEXT("Found duplicated StatTag in BaseStats in UFormStatComponent."));
	}
	//We set current stats to base stats and replicate since we delta serialize off it.
	CurrentStats.Items = BaseStats;
	CurrentStats.MarkArrayDirty();
	MARK_PROPERTY_DIRTY_FROM_NAME(UFormStatComponent, CurrentStats, this);
}
