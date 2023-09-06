// Fill out your copyright notice in the Description page of Project Settings.


#include "TestQuery.h"

UTestQuery::UTestQuery()
{
}

void UTestQuery::PerformCheck(float DeltaTime, UFormCoreComponent* FormCore)
{
	Super::PerformCheck(DeltaTime, FormCore);
	if (TestQueryDelegate.IsBound())
	{
		TestQueryDelegate.Broadcast();
	}
}

void UTestQuery::Initialize()
{
	Super::Initialize();
}

void UTestQuery::Deinitialize()
{
	Super::Deinitialize();
}
