﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfObject.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GameFramework/GameStateBase.h"
#include "UObject/Object.h"
#include "SfUtility.generated.h"

DECLARE_DYNAMIC_DELEGATE_SixParams(FOnEnterOverlapSignature, UPrimitiveComponent*, OverlappedComponent, AActor*,
								   OtherActor, UPrimitiveComponent*, OtherComp, int32, OtherBodyIndex, bool, bFromSweep,
								   const FHitResult &, SweepResult);

DECLARE_DYNAMIC_DELEGATE_FourParams(FOnExitOverlapSignature, UPrimitiveComponent*, OverlappedComponent, AActor*,
									OtherActor, UPrimitiveComponent*, OtherComp, int32, OtherBodyIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FClientVariableUpdateSignature);

template <class T>
bool TArrayCompareOrderless(const TArray<T>& A, const TArray<T>& B)
{
	//Early check to gate the more complicated checks.
	if (A.Num() != B.Num()) return false;
	if (A.Num() == 0) return true;
	TMap<T, TSet<T*>> SignatureGroupPairA = TArrayGroupEquivalentElements(A);
	TMap<T, TSet<T*>> SignatureGroupPairB = TArrayGroupEquivalentElements(B);
	//If one group has more types of elements return false.
	if (SignatureGroupPairA.Num() != SignatureGroupPairB.Num()) return false;
	//Compare each type of element and that the count is the same.
	for (TPair<T, TSet<T*>> Pair : SignatureGroupPairA)
	{
		if (SignatureGroupPairB[Pair.Key].Num() != Pair.Value.Num()) return false;
	}
	//Since we know the total count is the same, the group count is the same, and the count of each group is also
	//the same, we can assume the two arrays have equivalent elements.
	return true;
}

template <class T>
void TArrayGroupEquivalentProcessElement(T& Element, TMap<T, TSet<T*>>& OutSignatureGroupPairs)
{
	//Find the signature and add, otherwise create a new signature.
	for (TPair<T, TSet<T*>> Pair : OutSignatureGroupPairs)
	{
		if (Pair.Key != Element) continue;
		Pair.Value.Add(&Element);
		return;
	}
	OutSignatureGroupPairs.Add(Element, { &Element });
}

template <class T>
TMap<T, TSet<T*>> TArrayGroupEquivalentElements(TArray<T>& Array)
{
	TMap<T, TSet<T*>> SignatureGroupPairs;
	for (T& Element : Array)
	{
		TArrayGroupEquivalentProcessElement(Element, SignatureGroupPairs);
	}
	return SignatureGroupPairs;
}

template <class T, class F>
bool TArrayCheckDuplicate(const TArray<T>& Array, F&& Predicate)
{
	for (int32 i = 0; i < Array.Num() - 1; i++)
	{
		for (int32 j = i + 1; j < Array.Num(); j++)
		{
			if (Predicate(Array[i], Array[j]))
			{
				return true;
			}
		}
	}
	return false;
}

//Note that this will load all derived blueprint classes into memory.
inline TArray<UClass*> GetSubclassesOf(const TSubclassOf<UObject> ParentClass)
{
	TArray<UClass*> Subclasses;

	//Get native classes.
	
	GetDerivedClasses(ParentClass, Subclasses, true);

	//Remove blueprint classes.
	for (int64 i = Subclasses.Num() - 1; i >= 0; i--)
	{
		if (!Subclasses[i]->IsNative()) Subclasses.RemoveAt(i, 1, false);
	}
	Subclasses.Shrink();

	//Get blueprint classes.

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	TArray<FString> ContentPaths;
	ContentPaths.Add(TEXT("/Game/Blueprints"));
	AssetRegistry.ScanPathsSynchronous(ContentPaths);

	FTopLevelAssetPath BaseClassPath = ParentClass->GetClassPathName();

	//Get paths to derived classes.
	TSet< FTopLevelAssetPath > DerivedPaths;
	TArray< FTopLevelAssetPath > BasePaths;
	BasePaths.Add(BaseClassPath);
	TSet< FTopLevelAssetPath > Excluded;
	AssetRegistry.GetDerivedClassNames(BasePaths, Excluded, DerivedPaths);

	FARFilter Filter;
	const FTopLevelAssetPath BPPath(UBlueprint::StaticClass()->GetClassPathName());
	Filter.ClassPaths.Add(BPPath);
	Filter.bRecursiveClasses = true;

	TArray< FAssetData > AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	//Get classes from asset list.
	for (FAssetData& Asset : AssetList)
	{
		FAssetDataTagMapSharedView::FFindTagResult GeneratedClassPathPtr = Asset.TagsAndValues.FindTag("GeneratedClass");
		if (GeneratedClassPathPtr.IsSet())
		{
			const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(GeneratedClassPathPtr.GetValue());
			const FTopLevelAssetPath ClassPath = FTopLevelAssetPath(ClassObjectPath);

			if (!DerivedPaths.Contains(ClassPath)) continue;

			FString Name = Asset.GetObjectPathString() + TEXT("_C");
			Subclasses.Add(LoadObject<UClass>(nullptr, *Name));
		}
	}

	return Subclasses;
}

USTRUCT()
struct SFCORE_API FUint16_Quantize100
{
	GENERATED_BODY()
	
	UPROPERTY()
	uint16 InternalValue;
	
	FUint16_Quantize100();

	float GetFloat() const;

	void SetFloat(float Value);

	bool operator==(const FUint16_Quantize100& Other) const;

	friend FArchive& operator<<(FArchive& Ar, FUint16_Quantize100& Integer)
	{
		bool bDoSerialize = Integer.InternalValue != 0;
		Ar.SerializeBits(&bDoSerialize, 1);
		if (bDoSerialize)
		{
			Ar << Integer.InternalValue;
		}
		else if (Ar.IsLoading())
		{
			Integer.InternalValue = 0;
		}
		return Ar;
	}
};

USTRUCT()
struct SFCORE_API FInt16_Quantize10
{
	GENERATED_BODY()
	
	UPROPERTY()
	int16 InternalValue = 0;
	
	FInt16_Quantize10();

	float GetFloat() const;

	void SetFloat(float Value);

	bool operator==(const FInt16_Quantize10& Other) const;

	friend FArchive& operator<<(FArchive& Ar, FInt16_Quantize10& Integer)
	{
		bool bDoSerialize = Integer.InternalValue != 0;
		Ar.SerializeBits(&bDoSerialize, 1);
		if (bDoSerialize)
		{
			Ar << Integer.InternalValue;
		}
		else if (Ar.IsLoading())
		{
			Integer.InternalValue = 0;
		}
		return Ar;
	}
};

DECLARE_DELEGATE_OneParam(FSfTickFunction, const float);

USTRUCT()
struct SFCORE_API FSfTickFunctionHelper
{
	GENERATED_BODY()

	float Interval;

	float TickCurrentTime;

	float TickStartingTime;

	FSfTickFunction TickFunction;
	
	FSfTickFunctionHelper() : Interval(0), TickCurrentTime(0),
	                          TickStartingTime(0)
	{
	}

	template <class T>
	FSfTickFunctionHelper(T* InObjectToBind, void (T::*InTickFunction)(const float DeltaTime), const float InIntervalInSeconds) : FSfTickFunctionHelper()
	{
		TickFunction.BindUObject(InObjectToBind, InTickFunction);
		Interval = InIntervalInSeconds;
	}

	//Must be called by a higher tick function to drive the helper.
	void DriveTick(const float DeltaTime);
};

//For our use cases, a synchronized timestamp without latency compensation works well enough.

inline float CalculateFutureServerTimestamp(const UWorld* World, const float InAdditionalTime)
{
	if (!World)
	{
		UE_LOG(LogSfCore, Error, TEXT("Tried to call CalculateFutureServerTimestamp with null World."));
		return 0;
	}
	return World->GetGameState()->GetServerWorldTimeSeconds() + InAdditionalTime;
}

inline float CalculateTimeUntilServerTimestamp(const UWorld* World, const float InTimestamp)
{
	if (!World)
	{
		UE_LOG(LogSfCore, Error, TEXT("Tried to call CalculateTimeUntilServerTimestamp with null World."));
		return 0;
	}
	return InTimestamp - World->GetGameState()->GetServerWorldTimeSeconds();
}

inline float CalculateTimeSinceServerTimestamp(const UWorld* World,const float InTimestamp)
{
	if (!World)
	{
		UE_LOG(LogSfCore, Error, TEXT("Tried to call CalculateTimeSinceServerTimestamp with null World."));
		return 0;
	}
	return World->GetGameState()->GetServerWorldTimeSeconds() - InTimestamp;
}

inline bool HasServerTimestampPassed(const UWorld* World, const float InTimestamp)
{
	if (!World)
	{
		UE_LOG(LogSfCore, Error, TEXT("Tried to call HasServerTimestampPassed with null World."));
		return false;
	}
	return CalculateTimeUntilServerTimestamp(World, InTimestamp) <= 0;
}

