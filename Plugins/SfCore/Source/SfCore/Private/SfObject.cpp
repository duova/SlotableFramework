// Fill out your copyright notice in the Description page of Project Settings.


#include "SfObject.h"

#include "FormCharacterComponent.h"

DEFINE_LOG_CATEGORY(LogSfCore);

void USfObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	if (const UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass()))
	{
		BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}
}

bool USfObject::IsSupportedForNetworking() const
{
	return true;
}

int32 USfObject::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	if (!GetOuter())
	{
		UE_LOG(LogSfCore, Error, TEXT("USfObject class %s could not get outer for function callspace."),
		       *GetClass()->GetName());
		return 0;
	}
	return GetOuter()->GetFunctionCallspace(Function, Stack);
}

bool USfObject::CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack)
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		UE_LOG(LogSfCore, Error, TEXT("Attempted to call remote function on CDO for USfObject class %s."),
		       *GetClass()->GetName());
		return false;
	}
	if (!GetOwner()) return false;
	AActor* Owner = GetOwner();
	if (UNetDriver* NetDriver = Owner->GetNetDriver())
	{
		//Call using owing actor's NetDriver.
		NetDriver->ProcessRemoteFunction(Owner, Function, Parms, OutParms, Stack, this);
		return true;
	}
	return false;
}

void USfObject::Destroy()
{
	if (IsValid(this))
	{
		if (!GetOwner()) return;
		MarkAsGarbage();
	}
}

AActor* USfObject::SpawnActorInOwnerWorld(const TSubclassOf<AActor>& InClass, const FVector Location, const FRotator Rotation) const
{
	if (!GetOwner()) return nullptr;
	return GetOwner()->GetWorld()->SpawnActor<AActor>(InClass, Location, Rotation);
}

USfObject::USfObject()
{
}

bool USfObject::HasAuthority() const
{
	return GetOwner() && GetOwner()->HasAuthority();
}