// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SfCoreClasses.h"
#include "SfObject.generated.h"

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

	template<class T>
	static bool TArrayCompareOrderless(const TArray<T>& A, const TArray<T>& B);

	//Key is the element signature, value is a set of pointers to each element of the group.
	template<class T>
	static TMap<T, TSet<T*>> TArrayGroupEquivalentElements(const TArray<T>& Array);
	
	template<class T, class F>
	static bool TArrayCheckDuplicate(const TArray<T>& Array, F&& Predicate);

	inline static constexpr int32 Int32MaxValue = 2147483647;

protected:
	void ErrorIfNoAuthority() const;

private:
	UPROPERTY()
	UFormCharacterComponent* FormCharacterComponent;

	uint8 bDoesNotHaveFormCharacter:1;

	template<class T>
	static void TArrayGroupEquivalentProcessElement(const T Element, TMap<T, TSet<T*>>& SignatureGroupPairs);
};

template <class T>
bool USfObject::TArrayCompareOrderless(const TArray<T>& A, const TArray<T>& B)
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
TMap<T, TSet<T*>> USfObject::TArrayGroupEquivalentElements(const TArray<T>& Array)
{
	TMap<T, TSet<T*>> SignatureGroupPairs;
	for (T Element : Array)
	{
		TArrayGroupEquivalentProcessElement(Element, SignatureGroupPairs);
	}
	return SignatureGroupPairs;
}

template <class T, class F>
bool USfObject::TArrayCheckDuplicate(const TArray<T>& Array, F&& Predicate)
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

template <class T>
void USfObject::TArrayGroupEquivalentProcessElement(const T Element, TMap<T, TSet<T*>>& SignatureGroupPairs)
{
	//Find the signature and add, otherwise create a new signature.
	for (TPair<T, TSet<T*>> Pair : SignatureGroupPairs)
	{
		if (Pair.Key != Element) continue;
		Pair.Value.Add(&Element);
		return;
	}
	SignatureGroupPairs.Add(Element, { &Element });
}

USTRUCT()
struct SFCORE_API FUint16_Quantize100
{
	GENERATED_BODY()
	
	UPROPERTY()
	uint16 InternalValue;
	
	FUint16_Quantize100();

	float GetFloat();

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