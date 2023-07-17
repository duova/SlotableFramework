// Fill out your copyright notice in the Description page of Project Settings.


#include "FormStatComponent.h"
#include "Net/UnrealNetwork.h"


FStat::FStat(): Value(0)
{
}

FStat::FStat(const FGameplayTag StatTag, const float Value): StatTag(StatTag), Value(Value)
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

UFormStatComponent::UFormStatComponent()
{
	if (!GetOwner()) return;
	PrimaryComponentTick.bCanEverTick = false;
	CurrentStats.OwningFormStat = this;
	for (const FStat& BaseStat : BaseStats)
	{
		auto Duplicate = [BaseStat](const FStat& CheckedStat)
		{
			return CheckedStat.StatTag == BaseStat.StatTag && CheckedStat != BaseStat;
		};
		checkf(BaseStats.FindByPredicate(Duplicate) != nullptr,
		       TEXT("Each StatTag must be unique in BaseStats."));
	}
	//We set current stats to base stats and replicate since we delta serialize off it.
	CurrentStats.Items = BaseStats;
	CurrentStats.MarkArrayDirty();
}

void UFormStatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UFormStatComponent, CurrentStats);
}

bool UFormStatComponent::CalculateStat(const FGameplayTag& StatTag)
{
	const FStat* BaseStat = BaseStats.FindByPredicate([StatTag](const FStat& Stat){ return Stat.StatTag == StatTag;});
	FStat* CurrentStat = CurrentStats.Items.FindByPredicate([StatTag](const FStat& Stat)
	{
		return Stat.StatTag == StatTag;
	});
	//We don't need to calculate if stat doesn't exist in base or current stats.
	if (!BaseStat || !CurrentStat) return false;

	//We pass the value through all the modifiers in the order specified in the header file.
	float Value = BaseStat->Value;
	for (const FStat& AdditiveStat : AdditiveStatModifiers)
	{
		if (AdditiveStat.StatTag == StatTag)
		{
			//We clamp to a max value as beyond that we lose precision.
			Value = FMath::Clamp(Value + AdditiveStat.Value, 0, MaxValue);
		}
	}
	float ToMultiply = 1;
	for (const FStat& AdditiveMultiplicativeStat : AdditiveMultiplicativeStatModifiers)
	{
		if (AdditiveMultiplicativeStat.StatTag == StatTag)
		{
			ToMultiply = FMath::Clamp(ToMultiply + AdditiveMultiplicativeStat.Value, 0, MaxValue);
		}
	}
	Value = FMath::Clamp(Value * ToMultiply, 0, MaxValue);
	for (const FStat& TrueMultiplicativeStat : TrueMultiplicativeStatModifiers)
	{
		if (TrueMultiplicativeStat.StatTag == StatTag)
		{
			Value = FMath::Clamp(Value * TrueMultiplicativeStat.Value, 0, MaxValue);
		}
	}
	for (const FStat& FlatAdditiveStat : FlatAdditiveStatModifiers)
	{
		if (FlatAdditiveStat.StatTag == StatTag)
		{
			Value = FMath::Clamp(Value + FlatAdditiveStat.Value, 0, MaxValue);
		}
	}
	CurrentStat->Value = Value;
	CurrentStats.MarkItemDirty(*CurrentStat);
	return true;
}

const FStat& UFormStatComponent::Server_AddStatModifier(const FGameplayTag StatTag, const EStatModifierType Type,
	const float Value)
{
	const FStat* StatInstance = nullptr;
	switch (Type)
	{
	case Additive:
		StatInstance = &AdditiveStatModifiers[AdditiveStatModifiers.Emplace(StatTag, Value)];
		break;
	case AdditiveMultiplicative:
		StatInstance = &AdditiveMultiplicativeStatModifiers[AdditiveMultiplicativeStatModifiers.Emplace(StatTag, Value)];
		break;
	case TrueMultiplicative:
		StatInstance = &TrueMultiplicativeStatModifiers[TrueMultiplicativeStatModifiers.Emplace(StatTag, Value)];
		break;
	case FlatAdditive:
		StatInstance = &FlatAdditiveStatModifiers[FlatAdditiveStatModifiers.Emplace(StatTag, Value)];
		break;
	default:
		UE_LOG(LogTemp, Fatal, TEXT("Server_AddStatModifier was called with an invalid EStatModifierType."));
	}

	//This updates the relevant stat and sends updates to clients.
	CalculateStat(StatTag);
	checkf(StatInstance, TEXT("StatInstance is a nullptr."));

	//The stat instance is returned so it can be removed.
	return *StatInstance;
}

TArray<FStat> UFormStatComponent::Server_AddStatModifierBatch(const TArray<FStat> Stats, const EStatModifierType Type)
{
	TArray<FStat> ReturnStats;
	switch (Type)
	{
	case Additive:
		AdditiveStatModifiers.Reserve(Stats.Num());
		break;
	case AdditiveMultiplicative:
		AdditiveMultiplicativeStatModifiers.Reserve(Stats.Num());
		break;
	case TrueMultiplicative:
		TrueMultiplicativeStatModifiers.Reserve(Stats.Num());
		break;
	case FlatAdditive:
		FlatAdditiveStatModifiers.Reserve(Stats.Num());
		break;
	}
	ReturnStats.Reserve(Stats.Num());
	for (const FStat& Stat : Stats)
	{
		ReturnStats.Add(Server_AddStatModifier(Stat.StatTag, Type, Stat.Value));
	}

	//The stat instance is returned so it can be removed.
	return ReturnStats;
}

bool UFormStatComponent::Server_RemoveStatModifier(const FStat& Modifier, const EStatModifierType Type)
{
	bool bRemoved = false;
	switch (Type)
	{
	case Additive:
		bRemoved = AdditiveStatModifiers.RemoveSingle(Modifier) != 0;
		break;
	case AdditiveMultiplicative:
		bRemoved = AdditiveMultiplicativeStatModifiers.RemoveSingle(Modifier) != 0;
		break;
	case TrueMultiplicative:
		bRemoved = TrueMultiplicativeStatModifiers.RemoveSingle(Modifier) != 0;
		break;
	case FlatAdditive:
		bRemoved = FlatAdditiveStatModifiers.RemoveSingle(Modifier) != 0;
		break;
	default:
		return false;
	}
	
	//This updates the relevant stat.
	CalculateStat(Modifier.StatTag);
	return bRemoved;
}

bool UFormStatComponent::Server_RemoveStatModifierBatch(const TArray<FStat>& Modifiers, const EStatModifierType Type)
{
	//TODO: Optimize batch removal.
	bool bRemovedSomething = false;
	for (const FStat& Modifier : Modifiers)
	{
		bRemovedSomething |= Server_RemoveStatModifier(Modifier, Type);
	}
	return bRemovedSomething;
}

TArray<FStat> UFormStatComponent::GetCurrentStats()
{
	//Returning read only copy for BP.
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

void UFormStatComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetOwner()) return;
}
