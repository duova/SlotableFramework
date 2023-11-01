// Fill out your copyright notice in the Description page of Project Settings.


#include "SfGameMode.h"

FFormGroup::FFormGroup()
{
}

TArray<AActor*> ASfGameMode::GetActorsInTeam(const FGameplayTag InTeam)
{
	TArray<AActor*> ReturnArray;
	if (!TeamRegistry.Contains(InTeam)) return ReturnArray;
	ReturnArray.Reserve(TeamRegistry[InTeam].Members.Num());
	for (const UFormCoreComponent* FormCore : TeamRegistry[InTeam].Members)
	{
		ReturnArray.Add(FormCore->GetOwner());
	}
	return ReturnArray;
}

TArray<UFormCoreComponent*> ASfGameMode::GetFormCoresInTeam(const FGameplayTag InTeam)
{
	if (!TeamRegistry.Contains(InTeam)) return TArray<UFormCoreComponent*>();
	return TeamRegistry[InTeam].Members.Array();
}

void ASfGameMode::AddToTeam(UFormCoreComponent* InFormCore, const FGameplayTag& InTeam)
{
	if (!TeamRegistry.Contains(InTeam))
	{
		TeamRegistry.Add(InTeam, FFormGroup());
	}
	TeamRegistry[InTeam].Members.Add(InFormCore);
}

bool ASfGameMode::RemoveFromTeam(const UFormCoreComponent* InFormCore, const FGameplayTag& InTeam)
{
	if (!TeamRegistry.Contains(InTeam)) return false;
	return TeamRegistry[InTeam].Members.Remove(InFormCore) != 0;
}
