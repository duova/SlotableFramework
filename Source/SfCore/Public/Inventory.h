// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SfObject.h"
#include "Slotable.h"
#include "Card.h"
#include "Inventory.generated.h"

UENUM()
enum class EInventoryObjectType : uint8
{
	Null,
	Slotable,
	Card
};

/**
 * We use metadata to accomplish multiple things. We use classes to "predict" slotable and card creation and deletion
 * without actually creating or destroying them to avoid side effects. We also use this to predict the number of constituent
 * states and their order after a slotable addition or deletion to prevent desyncs. Finally, we use this to verify that the
 * correct cards are on the client.
 */
USTRUCT()
struct FInventoryObjectMetadata
{
	GENERATED_BODY()

	UPROPERTY()
	EInventoryObjectType Type;

	UPROPERTY()
	TSubclassOf<UObject> Class;

	UPROPERTY()
	uint8 InputEnabledConstituentsCount;

	FInventoryObjectMetadata();
	
	explicit FInventoryObjectMetadata(const TSubclassOf<USlotable> SlotableClass);

	explicit FInventoryObjectMetadata(const TSubclassOf<UCard> CardClass);

	bool operator==(const FInventoryObjectMetadata& Other) const;

	friend FArchive& operator<<(FArchive& Ar, FInventoryObjectMetadata& Metadata)
	{
		Ar.SerializeBits(&Metadata.Type, 2);
		Ar << Metadata.Class;
		Ar << Metadata.InputEnabledConstituentsCount;
		return Ar;
	}
};

/**
 * Inventories of slotables.
 * These can be dynamic or static, in terms of how many slotables they can contain.
 * They can also be preset with slotables, which can be locked.
 */
UCLASS(Blueprintable)
class SFCORE_API UInventory : public USfObject
{
	GENERATED_BODY()

public:
	UInventory();

	void Tick(float DeltaTime);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Read-only, non-predicted copy of slotables. Use HasSlotable instead for a predictive check.
	 */
	UFUNCTION(BlueprintGetter)
	TArray<USlotable*> GetSlotables();

	UFUNCTION(BlueprintGetter)
	TArray<USlotable*> GetSlotablesOfType(const TSubclassOf<USlotable>& SlotableClass);
	
	void InsertSlotableMetadataAtEnd(const TSubclassOf<USlotable>& SlotableClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	USlotable* Server_AddSlotable(const TSubclassOf<USlotable>& SlotableClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_RemoveSlotable(USlotable* Slotable, const bool bRemoveSlot);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Server_RemoveSlotableByIndex(const int32 Index, const bool bRemoveSlot);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	USlotable* Server_SetSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index, const bool bSlotMustBeNullOrEmpty);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	USlotable* Server_InsertSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_SwapSlotables(USlotable* SlotableA, USlotable* SlotableB);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Server_SwapSlotablesByIndex(const int32 IndexA, const int32 IndexB);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Server_TradeSlotablesBetweenInventories(USlotable* SlotableA, USlotable* SlotableB);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_AddCard(const TSubclassOf<UCard>& CardClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool Server_RemoveCard(const TSubclassOf<UCard>& CardClass);

	UFUNCTION(BlueprintCallable)
	bool Predicted_AddSlotable(const TSubclassOf<USlotable>& SlotableClass);

	UFUNCTION(BlueprintCallable)
	bool Predicted_RemoveSlotable(USlotable* Slotable, const bool bRemoveSlot);

	UFUNCTION(BlueprintCallable)
	void Predicted_RemoveSlotableByIndex(const int32 Index, const bool bRemoveSlot);
	
	UFUNCTION(BlueprintCallable)
	bool Predicted_SetSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index, const bool bSlotMustBeNullOrEmpty);

	UFUNCTION(BlueprintCallable)
	bool Predicted_InsertSlotable(const TSubclassOf<USlotable>& SlotableClass, const int32 Index);

	UFUNCTION(BlueprintCallable)
	bool Predicted_AddCard(const TSubclassOf<UCard>& CardClass);

	UFUNCTION(BlueprintCallable)
	bool Predicted_RemoveCard(const TSubclassOf<UCard>& CardClass);

	UFUNCTION(BlueprintPure)
	bool HasSlotable(const TSubclassOf<USlotable>& SlotableClass);

	UFUNCTION(BlueprintPure)
	uint8 SlotableCount(const TSubclassOf<USlotable>& SlotableClass);

	UFUNCTION(BlueprintPure)
	bool HasCard(const TSubclassOf<USlotable>& CardClass);

	//Only on client.
	UFUNCTION(BlueprintGetter)
	UCard* GetCard(const TSubclassOf<USlotable>& CardClass);

	void ClientInitialize();

	void ServerInitialize();
	
	void ClientDeinitialize();

	void ServerDeinitialize();

protected:

	virtual void BeginDestroy() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<USlotable>> InitialSlotableClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<UCard>> InitialCardClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<USlotable> EmptySlotableClass;

	//Whether slotables can be added.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 bIsDynamic:1;

	//Whether the slotables in the inventory can be changed.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 bIsChangeLocked:1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 Capacity = uint8();

	/**
	 * Called after a slotable is added to an inventory.
	 */
	void InitializeSlotable(USlotable* Slotable);

	/**
	 * Called before a slotable is removed from an inventory.
	 */
	void DeinitializeSlotable(USlotable* Slotable);

private:
	UPROPERTY(Replicated, VisibleAnywhere)
	TArray<USlotable*> Slotables;

	UPROPERTY(VisibleAnywhere)
	TArray<UCard*> Cards;

	//Does not synchronize to owner as owner should have it be predicted.
	//Ordered Slotable metadata is before unordered Card metadata.
	UPROPERTY(Replicated)
	TArray<FInventoryObjectMetadata> Metadata;

	//Compares with this to update cards on change.
	TArray<FInventoryObjectMetadata> ClientOldMetadata;

	USlotable* CreateUninitializedSlotable(const TSubclassOf<USlotable>& SlotableClass) const;

	UCard* CreateUninitializedCard(const TSubclassOf<UCard>& CardClass) const;

	void CheckAndUpdateCardsWithMetadata();
};
