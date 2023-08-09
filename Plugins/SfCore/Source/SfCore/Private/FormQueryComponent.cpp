// Fill out your copyright notice in the Description page of Project Settings.


#include "FormQueryComponent.h"

USfQuery::USfQuery()
{
}

void USfQuery::PerformCheck(float DeltaTime, UFormCoreComponent* FormCore)
{
}

void USfQuery::Initialize()
{
}

void USfQuery::Deinitialize()
{
}

UFormQueryComponent::UFormQueryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFormQueryComponent::RegisterQueryImpl(const TSubclassOf<USfQuery> InQueryClass)
{
	if (!GetOwner()) return;
	if (!InQueryClass.Get())
	{
		UE_LOG(LogSfCore, Error, TEXT("Empty QueryClass passed to RegisterQueryImpl."));
		return;
	}
	//We check if existing query exists.
	bool bHasQuery = false;
	for (TPair<USfQuery*, uint16>& Pair : ActiveQueryDependentCountPair)
	{
		if (Pair.Key->GetClass() == InQueryClass.Get())
		{
			bHasQuery = true;
			//Increment the number of dependents if it exists.
			Pair.Value++;
			break;
		}
	}
	if (!bHasQuery)
	{
		//Add query with 1 dependent if it doesn't.
		USfQuery* QueryInstance = NewObject<USfQuery>(GetOwner(), InQueryClass);
		QueryInstance->Initialize();
		ActiveQueryDependentCountPair.Add(QueryInstance, 1);
	}
}

void UFormQueryComponent::RegisterQueryDependencies(const TArray<TSubclassOf<USfQuery>>& InQueries)
{
	if (!GetOwner()) return;
	if (!GetOwner()->HasAuthority()) return;
	for (const TSubclassOf<USfQuery>& Query : InQueries)
	{
		if (!Query.Get())
		{
			UE_LOG(LogSfCore, Error, TEXT("Empty QueryClass passed to RegisterQueryDependencies."));
			continue;
		}
		RegisterQueryImpl(Query);
	}
}

void UFormQueryComponent::UnregisterQueryImpl(const TSubclassOf<USfQuery> InQueryClass)
{
	if (!InQueryClass.Get())
	{
		UE_LOG(LogSfCore, Error, TEXT("Empty QueryClass passed to UnregisterQueryImpl."));
		return;
	}
	for (auto It = ActiveQueryDependentCountPair.CreateIterator(); It; ++It)
	{
		if (It.Key()->GetClass() == InQueryClass.Get())
		{
			//Decrease the number of dependents only if it is more than 1.
			if (It.Value() > 1)
			{
				It.Value()--;
			}
			else
			{
				It.Key()->Deinitialize();
				It.RemoveCurrent();
			}
			break;
		}
	}
}

void UFormQueryComponent::UnregisterQueryDependencies(const TArray<TSubclassOf<USfQuery>>& InQueries)
{
	if (!GetOwner()) return;
	if (!GetOwner()->HasAuthority()) return;
	for (const TSubclassOf<USfQuery>& QueryClass : InQueries)
	{
		if (!QueryClass.Get())
		{
			UE_LOG(LogSfCore, Error, TEXT("Empty QueryClass passed to UnregisterQueryDependencies."));
			continue;
		}
		UnregisterQueryImpl(QueryClass);
	}
}

void UFormQueryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner()) return;
	if (!GetOwner()->HasAuthority()) return;
	for (const TPair<USfQuery*, uint16>& Pair : ActiveQueryDependentCountPair)
	{
		Pair.Key->PerformCheck(DeltaTime, FormCore);
	}
}

void UFormQueryComponent::SetupFormQuery(UFormCoreComponent* InFormCore)
{
	FormCore = InFormCore;
}

