// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SfCoreClasses.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "SfObject.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSfCore, Log, All);

/**
 * Base object for objects in the slotable hierarchy.
 * Routes remote function calls through the outer's NetDriver,
 * enables blueprint push model replication,
 * and provides utility functions.
 */
UCLASS()
class SFCORE_API USfObject : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	AActor* GetOwner() const
	{
		return GetTypedOuter<AActor>();
	}

	UFUNCTION(BlueprintPure)
	bool HasAuthority() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool IsSupportedForNetworking() const override;

	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;

	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Destroy();

	class UFormCharacterComponent* GetFormCharacter();
	
	bool IsFormCharacter();

	inline static constexpr int32 Int32MaxValue = 2147483647;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	AActor* SpawnActorInOwnerWorld(const TSubclassOf<AActor>& InClass, const FVector& Location, const FRotator& Rotation);

private:
	UPROPERTY()
	UFormCharacterComponent* FormCharacterComponent;

	uint8 bDoesNotHaveFormCharacter:1;
	
};

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

	//---Get native classes.---
	
	GetDerivedClasses(ParentClass, Subclasses, true);

	//Remove blueprint classes.
	for (int64 i = Subclasses.Num() - 1; i >= 0; i--)
	{
		if (!Subclasses[i]->IsNative()) Subclasses.RemoveAt(i, 1, false);
	}
	Subclasses.Shrink();

	//---Get blueprint classes.---

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