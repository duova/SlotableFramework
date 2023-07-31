// Fill out your copyright notice in the Description page of Project Settings.


#include "SfGauntletController.h"

#include "SfTestRunner.h"

void USfGauntletController::OnPostMapChange(UWorld* World)
{
	Super::OnPostMapChange(World);
	FString NetRole;
	if (World->GetNetMode() == NM_DedicatedServer)
	{
		NetRole = "DedicatedServer";
	}
	else if (World->GetNetMode() == NM_Client)
	{
		NetRole = "Client";
	}
	else if (World->GetNetMode() == NM_Standalone)
	{
		NetRole = "Standalone";
	}
	else if (World->GetNetMode() == NM_ListenServer)
	{
		NetRole = "ListenServer";
	}
	UE_LOG(LogGauntlet, Display, TEXT("Gauntlet controller loaded into world with role: %s"), *FString(NetRole));

	if (World->GetNetMode() == NM_DedicatedServer)
	{
		World->SpawnActor(ASfTestRunner::StaticClass());
	}
}
