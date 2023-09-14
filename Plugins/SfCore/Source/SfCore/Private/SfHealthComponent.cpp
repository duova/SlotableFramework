// Fill out your copyright notice in the Description page of Project Settings.


#include "SfHealthComponent.h"

#include "FormCoreComponent.h"
#include "FormStatComponent.h"
#include "Net/UnrealNetwork.h"

FHealthChangeData::FHealthChangeData(): InValue(0), OutValue(0), Source(nullptr), TimeoutTimestamp(0)
{
}

FHealthChangeData::FHealthChangeData(const float InValue, const float OutValue, UConstituent* Source,
                                   const TArray<TSubclassOf<UHealthChangeProcessor>>& InProcessors,
                                   const float InTimeoutTimestamp): InValue(InValue),
                                                                  OutValue(OutValue),
                                                                  Source(Source), Processors(InProcessors),
                                                                  TimeoutTimestamp(InTimeoutTimestamp)
{
}

bool FHealthChangeData::operator==(const FHealthChangeData& Other) const
{
	if (Other.InValue == InValue && Other.OutValue == OutValue && Other.Source == Source && Other.Processors ==
		Processors && Other.TimeoutTimestamp == TimeoutTimestamp) return true;
	return false;
}

bool FHealthChangeData::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
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
		for (TSubclassOf<UHealthChangeProcessor> Processor : Processors)
		{
			ProcessorClassIndicesToNetSerialize.Add(
				USfHealthComponent::AllHealthChangeProcessorClassesSortedByName.Find(Processor));
		}
	}
	Ar << ProcessorClassIndicesToNetSerialize;
	if (!Ar.IsSaving())
	{
		Processors.Empty();
		//We ID processors in a deterministic way to reduce bandwidth use.
		for (const uint8 ProcessorIndex : ProcessorClassIndicesToNetSerialize)
		{
			Processors.Add(USfHealthComponent::AllHealthChangeProcessorClassesSortedByName[ProcessorIndex]);
		}
	}
	return bOutSuccess;
}

UHealthChangeProcessor::UHealthChangeProcessor()
{
}

UDeathHandler::UDeathHandler()
{
}

void UDeathHandler::InternalServerOnDeath(USfHealthComponent* Health,
                                          const TArray<FHealthChangeData>& InOrderedRecentHealthChange)
{
	//Call BP functions to handle death on instances.
	Server_OnDeath(Health, InOrderedRecentHealthChange);
	InternalClientOnDeathRPC(Health, InOrderedRecentHealthChange);
}

void UDeathHandler::InternalClientOnDeathRPC_Implementation(USfHealthComponent* Health,
                                                            const TArray<FHealthChangeData>& InOrderedRecentHealthChange)
{
	Autonomous_OnDeath(Health, InOrderedRecentHealthChange);
}

USfHealthComponent::USfHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bReplicateUsingRegisteredSubObjectList = true;
	SetIsReplicatedByDefault(true);
}

void USfHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetOwner()) return;
	//If this is 0 we don't even get data about who killed the owner as it would get instantly timed out.
	if (RecentHealthChangeDataTimeout <= 0)
	{
		UE_LOG(LogSfCore, Error, TEXT("RecentHealthChangeDataTimeout on SfHealthComponent class %s should be more than 0."), *GetClass()->GetName());
	}
	//Assign all health change processors an ID for net serialization.
	if (!HealthChangeProcessorClassesFetched)
	{
		AllHealthChangeProcessorClassesSortedByName = GetSubclassesOf(UHealthChangeProcessor::StaticClass());
		for (int64 i = AllHealthChangeProcessorClassesSortedByName.Num() - 1; i >= 0; i--)
		{
			if (AllHealthChangeProcessorClassesSortedByName[i]->HasAnyClassFlags(CLASS_Abstract) || AllHealthChangeProcessorClassesSortedByName[i] == UHealthChangeProcessor::StaticClass())
			{
				AllHealthChangeProcessorClassesSortedByName.RemoveAt(i, 1, false);
			}
		}
		AllHealthChangeProcessorClassesSortedByName.Shrink();
		//We sort this in a deterministic order to index items.
		AllHealthChangeProcessorClassesSortedByName.Sort([](const UClass& A, const UClass& B)
		{
			return A.GetPathName() > B.GetPathName();
		});
		if (AllHealthChangeProcessorClassesSortedByName.Num() > 255)
		{
			UE_LOG(LogSfCore, Error, TEXT("Only 255 health change processors can exist. Removing excess."));
			while (AllHealthChangeProcessorClassesSortedByName.Num() > 255)
			{
				AllHealthChangeProcessorClassesSortedByName.RemoveAt(256, 1, false);
			}
			AllHealthChangeProcessorClassesSortedByName.Shrink();
		}
		HealthChangeProcessorClassesFetched = true;
	}
	CalculatedTimeBetweenHealthUpdates = 1.f / HealthConstantUpdatesPerSecond;
	if (FormCore) return;
	//Only run if form core has not been set so SetupSfHeath called by FormCore will either not run (if it doesn't exist)
	//or overwrite Health with the correct value. We make sure we don't overwrite FormCore's max health value since that is
	//always correct.
	Health = GetMaxHealth();
	MARK_PROPERTY_DIRTY_FROM_NAME(USfHealthComponent, Health, this);
}

void USfHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//Apply consistent health changes.
	if (!GetOwner()->HasAuthority() || !FormStat) return;
	//Don't apply constant changes if the form is dead, they must be revived directly.
	if (Health <= 0) return;
	HealthUpdateTimer += DeltaTime;
	if (HealthUpdateTimer > CalculatedTimeBetweenHealthUpdates)
	{
		HealthUpdateTimer -= CalculatedTimeBetweenHealthUpdates;
		const float HealthChange = FormCore->GetFormStat()->GetStat(HealthRegenerationPerSecondStat) - FormCore->
			GetFormStat()->GetStat(HealthDegenerationPerSecondStat);
		ApplyHealthChange(HealthChange * CalculatedTimeBetweenHealthUpdates, nullptr, TArray<TSubclassOf<UHealthChangeProcessor>>());
	}
}

void USfHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams DefaultParams;
	DefaultParams.bIsPushBased = true;
	DefaultParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(USfHealthComponent, Health, DefaultParams);
}

float USfHealthComponent::ApplyHealthChange(const float InValue, UConstituent* Source,
                                           const TArray<TSubclassOf<UHealthChangeProcessor>>& InProcessors)
{
	if (!GetOwner()->HasAuthority()) return 0;
	//We pass the value through all processors.
	float ProcessedValue = InValue;
	for (const TSubclassOf<UHealthChangeProcessor>& Processor : InProcessors)
	{
		UHealthChangeProcessor* ProcessorCDO = Processor.GetDefaultObject();
		//This logic applies modifications from each process to ProcessedValue in a serial way.
		float LocalProcessedValue = 0;
		ProcessorCDO->ProcessHealthChange(ProcessedValue, Source, this, LocalProcessedValue);
		ProcessedValue = LocalProcessedValue;
	}
	
	const float OriginalHealth = Health;
	float NewHealth = FMath::Clamp(Health + ProcessedValue, 0, GetMaxHealth());

	//Broadcast delegate to allow for intervening or calling other events.
	Server_OnHealthChange.Broadcast(OriginalHealth, NewHealth, Source, InProcessors);

	Health = NewHealth;
	MARK_PROPERTY_DIRTY_FROM_NAME(USfHealthComponent, Health, this);

	//We want to return the actual health change, not the processed value.
	const float FinalHealthChange = Health - OriginalHealth;

	//We compress health change data with identical sources and processors together in order to prevent ticked
	//health changes (damage over time) to bloat our buffer.
	AddHealthChangeDataAndCompress(InValue, FinalHealthChange, Source, InProcessors,
	                              FormCore->CalculateFutureServerFormTimestamp(RecentHealthChangeDataTimeout));
	TrimTimeout();
	if (Health <= 0)
	{
		DeathHandlerClass.GetDefaultObject()->InternalServerOnDeath(this, OrderedRecentHealthChange);
	}
	return FinalHealthChange;
}

float USfHealthComponent::ApplyHealthChangeFractionOfMax(const float InValue, UConstituent* Source,
                                                        const TArray<TSubclassOf<UHealthChangeProcessor>>& InProcessors)
{
	if (!GetOwner()->HasAuthority()) return 0;
	const float ActualValue = InValue * GetMaxHealth();
	return ApplyHealthChange(ActualValue, Source, InProcessors);
}

float USfHealthComponent::ApplyHealthChangeFractionOfRemaining(const float InValue, UConstituent* Source,
                                                              const TArray<TSubclassOf<UHealthChangeProcessor>>&
                                                              InProcessors)
{
	if (!GetOwner()->HasAuthority()) return 0;
	const float ActualValue = InValue * Health;
	return ApplyHealthChange(ActualValue, Source, InProcessors);
}

float USfHealthComponent::GetHealth() const
{
	return Health;
}

float USfHealthComponent::GetMaxHealth()
{
	if (!FormCore) return MaxHealthFallback;
	if (!FormCore->GetFormStat()) return MaxHealthFallback;
	const float MaxHealthStatValue = FormCore->GetFormStat()->GetStat(MaxHealthStat);
	if (MaxHealthStatValue > 0) return MaxHealthStatValue;
	return MaxHealthFallback;
}

UFormCoreComponent* USfHealthComponent::GetFormCore() const
{
	return FormCore;
}

void USfHealthComponent::SetupSfHealth(UFormCoreComponent* InFormCore)
{
	FormCore = InFormCore;
}

void USfHealthComponent::SecondarySetupSfHealth()
{
	if (!FormCore) return;
	FormStat = FormCore->GetFormStat();
	Health = GetMaxHealth();
	MARK_PROPERTY_DIRTY_FROM_NAME(USfHealthComponent, Health, this);
}

const void USfHealthComponent::AddHealthChangeDataAndCompress(
	const float InValue, const float OutValue, UConstituent* Source,
	const TArray<TSubclassOf<UHealthChangeProcessor>>& InProcessors, const float InTimeoutTimestamp)
{
	FHealthChangeData SimilarData;
	bool bFoundSimilarData = false;
	for (FHealthChangeData& Data : OrderedRecentHealthChange)
	{
		//Pack new data into existing data if possible.
		if (Data.Processors == InProcessors && Data.Source == Source)
		{
			Data.InValue += InValue;
			Data.OutValue += OutValue;
			//Refresh timeout.
			Data.TimeoutTimestamp = InTimeoutTimestamp;
			SimilarData = Data;
			bFoundSimilarData = true;
			break;
		}
	}
	if (bFoundSimilarData)
	{
		//Move to the front since it becomes the most recent.
		if (OrderedRecentHealthChange.Num() > 1)
		{
			OrderedRecentHealthChange.Swap(0, OrderedRecentHealthChange.Find(SimilarData));
		}
		return;
	}
	OrderedRecentHealthChange[OrderedRecentHealthChange.Insert(
		FHealthChangeData(InValue, OutValue, Source, InProcessors, InTimeoutTimestamp), 0)];
}

void USfHealthComponent::TrimTimeout()
{
	for (int16 i = OrderedRecentHealthChange.Num() - 1; i >= 0; i--)
	{
		if (FormCore->HasServerFormTimestampPassed(OrderedRecentHealthChange[i].TimeoutTimestamp))
		{
			OrderedRecentHealthChange.RemoveAt(i, 1, false);
		}
	}
	OrderedRecentHealthChange.Shrink();
}
