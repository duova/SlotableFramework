// Fill out your copyright notice in the Description page of Project Settings.


#include "SfObject.h"

#include "FormCharacterComponent.h"

DEFINE_LOG_CATEGORY(LogSfCore);

void USfObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//Get blueprint class replicated properties and make them push based.
	if (const UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass()))
	{
		TArray<FLifetimeProperty> BPLifetimeProps;
		BPClass->GetLifetimeBlueprintReplicationList(BPLifetimeProps);
		for (FLifetimeProperty& Prop : BPLifetimeProps)
		{
			Prop.bIsPushBased = true;
			OutLifetimeProps.AddUnique(Prop);
		}
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

bool USfObject::IsFormCharacter()
{
	if (FormCharacterComponent) return true;
	if (bDoesNotHaveFormCharacter) return false;
	if (!GetOwner()) return false;
	if (UFormCharacterComponent* Component = Cast<UFormCharacterComponent>(
		GetOwner()->FindComponentByClass(UFormCharacterComponent::StaticClass())))
	{
		FormCharacterComponent = Component;
		bDoesNotHaveFormCharacter = false;
		return true;
	}
	bDoesNotHaveFormCharacter = true;
	return false;
}

UFormCharacterComponent* USfObject::GetFormCharacter()
{
	if (FormCharacterComponent) return FormCharacterComponent;
	if (bDoesNotHaveFormCharacter) return nullptr;
	if (!GetOwner()) return nullptr;
	if (UFormCharacterComponent* Component = Cast<UFormCharacterComponent>(
		GetOwner()->FindComponentByClass(UFormCharacterComponent::StaticClass())))
	{
		FormCharacterComponent = Component;
		bDoesNotHaveFormCharacter = false;
	}
	else
	{
		bDoesNotHaveFormCharacter = true;
	}
	return Cast<UFormCharacterComponent>(GetOwner()->FindComponentByClass(UFormCharacterComponent::StaticClass()));
}

bool USfObject::HasAuthority() const
{
	return GetOwner() && GetOwner()->HasAuthority();
}

FUint16_Quantize100::FUint16_Quantize100(): InternalValue(0)
{
}

float FUint16_Quantize100::GetFloat() const
{
	return static_cast<float>(InternalValue) / 100.0;
}

void FUint16_Quantize100::SetFloat(const float Value)
{
	if (Value > 655.35)
	{
		InternalValue = 65535;
		return;
	}
	if (Value < 0)
	{
		InternalValue = 0;
		return;
	}
	InternalValue = static_cast<uint8>(Value * 100.0);
}

bool FUint16_Quantize100::operator==(const FUint16_Quantize100& Other) const
{
	if (InternalValue == Other.InternalValue) return true;
	return false;
}