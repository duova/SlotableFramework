// Fill out your copyright notice in the Description page of Project Settings.


#include "SfGauntletController.h"

#include "SfTestRunner.h"

void USfGauntletController::OnPostMapChange(UWorld* World)
{
	Super::OnPostMapChange(World);

	//Print what role a instance is when it changes to a new world.
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

	//Connect clients if not connected.
	if (World->GetNetMode() == NM_Standalone)
	{
		World->GetFirstPlayerController()->ClientTravel("127.0.0.1", TRAVEL_Absolute);
	}

	//Spawn the test runner on the server.
	if (World->GetNetMode() == NM_DedicatedServer)
	{
		ServerTestRunner = static_cast<ASfTestRunner*>(World->SpawnActor(ASfTestRunner::StaticClass()));
		ServerTestRunner->ServerSfGauntletController = this;
	}
}

void USfGauntletController::SfEndSession()
{
	EndTest(0);
}
