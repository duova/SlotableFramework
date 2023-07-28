// Fill out your copyright notice in the Description page of Project Settings.


#include "SfHealthComponent.h"

#include "FormCoreComponent.h"
#include "FormStatComponent.h"
#include "Net/UnrealNetwork.h"

FHealthDeltaData::FHealthDeltaData(): InValue(0), OutValue(0), Source(nullptr), TimeoutTimestamp(0)
{
}

FHealthDeltaData::FHealthDeltaData(const float InValue, const float OutValue, UConstituent* Source,
                                   const TArray<TSubclassOf<UHealthDeltaProcessor>>& Processors,
                                   const float TimeoutTimestamp): InValue(InValue),
                                                                  OutValue(OutValue),
                                                                  Source(Source), Processors(Processors),
                                                                  TimeoutTimestamp(TimeoutTimestamp)
{
}

bool FHealthDeltaData::operator==(const FHealthDeltaData& Other) const
{
	if (Other.InValue == InValue && Other.OutValue == OutValue && Other.Source == Source && Other.Processors ==
		Processors && Other.TimeoutTimestamp == TimeoutTimestamp) return true;
	return false;
}

bool FHealthDeltaData::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	Ar << InValue;
	Ar << OutValue;
	UObject* SourceToNetSerialize = Source;
	bOutSuccess = Map->SerializeObject(Ar, UConstituent::StaticClass(), SourceToNetSerialize);
	if (!Ar.IsSaving() && SourceToNetSerialize)
	{
		Source = Cast<UConstituent>(SourceToNetSerialize);
	}
	TArray<uint8> ProcessorClassIndicesToNetSerialize;
	if (Ar.IsSaving())
	{
		for (TSubclassOf<UHealthDeltaProcessor> Processor : Processors)
		{
			ProcessorClassIndicesToNetSerialize.Add(
				USfHealthComponent::AllHealthDeltaProcessorClassesSortedByName.Find(Processor));
		}
	}
	Ar << ProcessorClassIndicesToNetSerialize;
	if (!Ar.IsSaving())
	{
		Processors.Empty();
		//We ID processors in a deterministic way to reduce bandwidth use.
		for (const uint8 ProcessorIndex : ProcessorClassIndicesToNetSerialize)
		{
			Processors.Add(USfHealthComponent::AllHealthDeltaProcessorClassesSortedByName[ProcessorIndex]);
		}
	}
	return bOutSuccess;
}

UHealthDeltaProcessor::UHealthDeltaProcessor()
{
}

UDeathHandler::UDeathHandler()
{
}

void UDeathHandler::InternalServerOnDeath(USfHealthComponent* Health,
                                          const TArray<FHealthDeltaData>& OrderedRecentHealthDelta)
{
	//Call BP functions to handle death on instances.
	Server_OnDeath(Health, OrderedRecentHealthDelta);
	InternalClientOnDeathRPC(Health, OrderedRecentHealthDelta);
}

void UDeathHandler::InternalClientOnDeathRPC_Implementation(USfHealthComponent* Health,
                                                            const TArray<FHealthDeltaData>& OrderedRecentHealthDelta)
{
	Autonomous_OnDeath(Health, OrderedRecentHealthDelta);
}

USfHealthComponent::USfHealthComponent()
{
	if (!GetOwner()) return;
	PrimaryComponentTick.bCanEverTick = false;
	bReplicateUsingRegisteredSubObjectList = true;
	SetIsReplicated(true);
}

void USfHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetOwner()) return;
	//If this is 0 we don't even get data about who killed the owner as it would get instantly timed out.
	checkf(RecentHealthDeltaDataTimeout > 0, TEXT("RecentHealthDeltaDataTimeout should be more than 0."))
	//Assign all health delta processors an ID for net serialization.
	if (!HealthDeltaProcessorClassesFetched)
	{
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (It->IsChildOf(UHealthDeltaProcessor::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract))
			{
				AllHealthDeltaProcessorClassesSortedByName.Add(*It);
			}
		}
		//We sort this in a deterministic order to index items.
		checkf(AllHealthDeltaProcessorClassesSortedByName.Num() < 256,
		       TEXT("Only 255 health delta processors can exist."))
		AllHealthDeltaProcessorClassesSortedByName.Sort([](const UClass& A, const UClass& B)
		{
			return A.GetFullName() > B.GetFullName();
		});
		HealthDeltaProcessorClassesFetched = true;
	}
	if (FormCore) return;
	//Only run if form core has not been set so SetupSfHeath called by FormCore will either not run (if it doesn't exist)
	//or overwrite Health with the correct value. We make sure we don't overwrite FormCore's max health value since that is
	//always correct.
	Health = GetMaxHealth();
}

void USfHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(USfHealthComponent, Health, DefaultParams);
}

float USfHealthComponent::ApplyHealthDelta(const float Value, UConstituent* Source,
                                           TArray<TSubclassOf<UHealthDeltaProcessor>> Processors)
{
	//We pass the value through all processors.
	float ProcessedValue = Value;
	for (TSubclassOf<UHealthDeltaProcessor> Processor : Processors)
	{
		UHealthDeltaProcessor* ProcessorCDO = Processor.GetDefaultObject();
		//This logic applies modifications from each process to ProcessedValue in a serial way.
		float LocalProcessedValue = 0;
		ProcessorCDO->ProcessHealthDelta(ProcessedValue, Source, this, LocalProcessedValue);
		ProcessedValue = LocalProcessedValue;
	}
	
	const float OriginalHealth = Health;
	float NewHealth = FMath::Clamp(Health + ProcessedValue, 0, GetMaxHealth());

	//Broadcast delegate to allow for intervening or calling other events.
	Server_OnHealthChange.Broadcast(OriginalHealth, NewHealth, Source, Processors);

	Health = NewHealth;

	//We want to return the actual health change, not the processed value.
	const float FinalHealthDelta = Health - OriginalHealth;

	//We compress health delta data with identical sources and processors together in order to prevent ticked
	//health changes (damage over time) to bloat our buffer.
	AddHealthDeltaDataAndCompress(Value, FinalHealthDelta, Source, Processors,
	                              FormCore->CalculateFutureServerTimestamp(RecentHealthDeltaDataTimeout));
	TrimTimeout();
	if (Health <= 0)
	{
		DeathHandlerClass.GetDefaultObject()->InternalServerOnDeath(this, OrderedRecentHealthDelta);
	}
	return FinalHealthDelta;
}

float USfHealthComponent::ApplyHealthDeltaFractionOfMax(const float Value, UConstituent* Source,
                                                        const TArray<TSubclassOf<UHealthDeltaProcessor>> Processors)
{
	const float ActualValue = Value * GetMaxHealth();
	return ApplyHealthDelta(ActualValue, Source, Processors);
}

float USfHealthComponent::ApplyHealthDeltaFractionOfRemaining(const float Value, UConstituent* Source,
                                                              const TArray<TSubclassOf<UHealthDeltaProcessor>>
                                                              Processors)
{
	const float ActualValue = Value * Health;
	return ApplyHealthDelta(ActualValue, Source, Processors);
}

float USfHealthComponent::GetHealth() const
{
	return Health;
}

float USfHealthComponent::GetMaxHealth()
{
	if (!FormCore) return MaxHealthOverride;
	if (!FormCore->GetFormStat()) return MaxHealthOverride;
	const float MaxHealthStat = FormCore->GetFormStat()->GetStat(MaxHealthStatTag);
	if (MaxHealthStat > 0) return MaxHealthStat;
	return MaxHealthOverride;
}

UFormCoreComponent* USfHealthComponent::GetFormCore() const
{
	return FormCore;
}

void USfHealthComponent::SetupSfHealth(UFormCoreComponent* InFormCore)
{
	FormCore = InFormCore;
	Health = GetMaxHealth();
}

const FHealthDeltaData& USfHealthComponent::AddHealthDeltaDataAndCompress(
	const float InValue, const float OutValue, UConstituent* Source,
	const TArray<TSubclassOf<UHealthDeltaProcessor>>& Processors, const float TimeoutTimestamp)
{
	FHealthDeltaData SimilarData;
	bool bFoundSimilarData = false;
	for (FHealthDeltaData& Data : OrderedRecentHealthDelta)
	{
		//Pack new data into existing data if possible.
		if (Data.Processors == Processors && Data.Source == Source)
		{
			Data.InValue += InValue;
			Data.OutValue += OutValue;
			//Refresh timeout.
			Data.TimeoutTimestamp = TimeoutTimestamp;
			SimilarData = Data;
			bFoundSimilarData = true;
			break;
		}
	}
	if (bFoundSimilarData)
	{
		//Move to the front since it becomes the most recent.
		OrderedRecentHealthDelta.Remove(SimilarData);
		OrderedRecentHealthDelta.Insert(SimilarData, 0);
		return SimilarData;
	}
	return OrderedRecentHealthDelta[OrderedRecentHealthDelta.Insert(
		FHealthDeltaData(InValue, OutValue, Source, Processors, TimeoutTimestamp), 0)];
}

void USfHealthComponent::TrimTimeout()
{
	TArray<FHealthDeltaData> ToRemove;
	for (const FHealthDeltaData& Data : OrderedRecentHealthDelta)
	{
		if (FormCore->HasServerTimestampPassed(Data.TimeoutTimestamp))
		{
			ToRemove.Add(Data);
		}
	}
	for (const FHealthDeltaData& Data : ToRemove)
	{
		OrderedRecentHealthDelta.Remove(Data);
	}
}
