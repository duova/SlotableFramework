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

void UFormQueryComponent::RegisterQueryImpl(const TSubclassOf<USfQuery> QueryClass)
{
	if (!GetOwner()) return;
	//We check if existing query exists.
	bool bHasQuery = false;
	for (TPair<USfQuery*, uint16>& Pair : ActiveQueryDependentCountPair)
	{
		if (Pair.Key->GetClass() == QueryClass.Get())
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
		USfQuery* QueryInstance = NewObject<USfQuery>(GetOwner(), QueryClass);
		QueryInstance->Initialize();
		ActiveQueryDependentCountPair.Add(QueryInstance, 1);
	}
}

void UFormQueryComponent::RegisterQueryDependencies(const TArray<TSubclassOf<USfQuery>>& Queries)
{
	if (!GetOwner()) return;
	if (!GetOwner()->HasAuthority()) return;
	for (const TSubclassOf<USfQuery> Query : Queries)
	{
		RegisterQueryImpl(Query);
	}
}

void UFormQueryComponent::UnregisterQueryImpl(const TSubclassOf<USfQuery> QueryClass)
{
	USfQuery* ToRemove = nullptr;
	for (TPair<USfQuery*, uint16>& Pair : ActiveQueryDependentCountPair)
	{
		if (Pair.Key->GetClass() == QueryClass.Get())
		{
			//Decrease the number of dependents only if it is more than 1.
			if (Pair.Value > 1)
			{
				Pair.Value--;
			}
			else
			{
				ToRemove = Pair.Key;
			}
			break;
		}
	}
	if (ToRemove)
	{
		ToRemove->Deinitialize();
		ActiveQueryDependentCountPair.Remove(ToRemove);
	}
}

void UFormQueryComponent::UnregisterQueryDependencies(const TArray<TSubclassOf<USfQuery>>& Queries)
{
	if (!GetOwner()) return;
	if (!GetOwner()->HasAuthority()) return;
	for (const TSubclassOf<USfQuery> QueryClass : Queries)
	{
		UnregisterQueryImpl(QueryClass);
	}
}

void UFormQueryComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetOwner()) return;
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
