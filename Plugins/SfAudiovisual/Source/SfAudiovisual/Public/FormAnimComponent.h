// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FormAnimComponent.generated.h"

enum class EPerspective : uint8;
class UFormCharacterComponent;

USTRUCT()
struct SFAUDIOVISUAL_API FRecentMontageData
{
	GENERATED_BODY()

	FRecentMontageData();

	FRecentMontageData(UAnimMontage* InMontage, const float InStartTimestamp,
	                   const UFormCharacterComponent* InFormCharacterComponent);

	UPROPERTY()
	UAnimMontage* Montage;

	float StartTimestamp;

	//-1 means looping.
	float ExpectedEndTimestamp;

	//-1 means not interrupted.
	float InterruptedTimestamp;
};

USTRUCT()
struct SFAUDIOVISUAL_API FTimestampedPoseSnapshot
{
	GENERATED_BODY()

	FTimestampedPoseSnapshot();
	
	float Timestamp;

	FPoseSnapshot Snapshot;
};

class UFormCoreComponent;

/**
 * Component that allows constituents to make predicted and non-predicted animation montage calls.
 * Details:
 * Use [NetRole]_Play[Perspective]Montage function to make a anim montage call for a graph with the role and perspective.
 * Note that Predicted only plays the montage on the owning client predictively, but not on the server as might be expected.
 * Separate calls should be made on the server and simulated graphs on the same action for hit registration and for other
 * players to see the animation.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SFAUDIOVISUAL_API UFormAnimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFormAnimComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
										FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static UFormAnimComponent* GetFormAnimComponent(UFormCoreComponent* InFormCore);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	USkeletalMeshComponent* ThirdPersonSkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	USkeletalMeshComponent* FirstPersonSkeletalMesh;

	//Must be used with corresponding constituent action graph.
	//Plays montage predictively for the chosen skeletal mesh on the owning client but not on the server.
	UFUNCTION(BlueprintCallable)
	void Predicted_PlayMontage(UAnimMontage* InMontage, const EPerspective InPerspective,
	                           const bool bInStopAllMontages);

	//Must be used with corresponding constituent action graph.
	//Used only for hit registration.
	//Plays montage for the third person skeletal mesh on the server.
	UFUNCTION(BlueprintCallable)
	void Server_PlayMontage(UAnimMontage* InMontage, const bool bInStopAllMontages);

	//Must be used with corresponding constituent action graph.
	//Plays montage non-predictively for the chosen skeletal mesh on the owning client only.
	UFUNCTION(BlueprintCallable)
	void Autonomous_PlayMontage(const float InTimeSinceExecution, UAnimMontage* InMontage,
	                            const EPerspective InPerspective,
	                            const bool bInStopAllMontages);

	//Must be used with corresponding constituent action graph.
	//Plays montage non-predictively for the chosen skeletal mesh on non-owning clients only.
	UFUNCTION(BlueprintCallable)
	void Simulated_PlayMontage(const float InTimeSinceExecution, UAnimMontage* InMontage,
	                           const EPerspective InPerspective,
	                           const bool bInStopAllMontages);

	//Must be used with corresponding constituent action graph.
	//Stops montage predictively for the chosen skeletal mesh on the owning client but not on the server.
	UFUNCTION(BlueprintCallable)
	void Predicted_StopMontage(UAnimMontage* InMontage, const EPerspective InPerspective,
	                           const float InBlendOutTime);

	//Must be used with corresponding constituent action graph.
	//Used only for hit registration.
	//Stops montage for the third person skeletal mesh on the server.
	UFUNCTION(BlueprintCallable)
	void Server_StopMontage(UAnimMontage* InMontage, const float InBlendOutTime);

	//Must be used with corresponding constituent action graph.
	//Stops montage non-predictively for the chosen skeletal mesh on the owning client only.
	UFUNCTION(BlueprintCallable)
	void Autonomous_StopMontage(const float InTimeSinceExecution, UAnimMontage* InMontage,
	                            const EPerspective InPerspective, const float InBlendOutTime);

	//Must be used with corresponding constituent action graph.
	//Stops montage non-predictively for the chosen skeletal mesh on non-owning clients only.
	UFUNCTION(BlueprintCallable)
	void Simulated_StopMontage(const float InTimeSinceExecution, UAnimMontage* InMontage,
	                           const EPerspective InPerspective, const float InBlendOutTime);

	//For lag compensated hit registration.
	void ServerRollbackPose(const float FormTimestamp);

	//Must be called after ServerRollbackAnimation to bring animation back to current time.
	void ServerRestoreLatestPose();

	//Evaluate animation every x anim frames for server hit registration.
	//Anim FPS is generally 30, and this must be a factor ie. 3, 6, 10, 15.
	UPROPERTY(EditAnywhere, meta = (ClampMin = 0, ClampMax = 30))
	int32 ServerAnimRate = 6;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void InternalPredictedPlayMontage(UAnimMontage* InMontage, const bool bInStopAllMontages,
	                                  const bool bIsFirstPerson);

	void InternalServerPlayMontage(UAnimMontage* InMontage, const bool bInStopAllMontages);

	void InternalClientPlayMontage(const float InTimeSinceExecution, UAnimMontage* InMontage,
	                               const bool bInStopAllMontages, const bool bIsFirstPerson);

	void InternalPredictedStopMontage(UAnimMontage* InMontage, const bool bIsFirstPerson,
	                                  const float InBlendOutTime);

	void InternalServerStopMontage(UAnimMontage* InMontage, const float InBlendOutTime);

	void InternalClientStopMontage(const float InTimeSinceExecution, UAnimMontage* InMontage, const bool bIsFirstPerson,
	                               const float InBlendOutTime);

	UFUNCTION()
	void ReplayBegin();

	UFUNCTION()
	void ReplayEnd();

	void CleanUpIrreleventMontageData(TArray<FRecentMontageData>& RecentMontages);

	void RecreateMontagesFromRecentData(UAnimInstance* Instance, const TArray<FRecentMontageData>& InRecentMontages,
	                                    const UFormCharacterComponent* InFormCharacterComponent);

	void RollBackMontageData(TArray<FRecentMontageData>& InRecentMontages, const UFormCharacterComponent* InFormCharacterComponent);

	void ReproducePoseSnapshot(const FPoseSnapshot& Snapshot, USkeletalMeshComponent* SkeletalMeshComponent);

	FDelegateHandle ReplayBeginDelegateHandle;

	FDelegateHandle ReplayEndDelegateHandle;

	UPROPERTY()
	UFormCharacterComponent* FormCharacterComponent;

	UPROPERTY()
	UFormCoreComponent* FormCoreComponent;

	UAnimInstance* GetAnimInstanceChecked(const USkeletalMeshComponent* SkeletalMeshComponent) const;

	uint8 bReplaying : 1;

	//Not in time order as we remove with swap.
	TArray<FRecentMontageData> FirstPersonRecentMontages;

	//Not in time order as we remove with swap.
	TArray<FRecentMontageData> ThirdPersonRecentMontages;

	TArray<FTimestampedPoseSnapshot> ServerAnimSnapshots;

	float TimeSinceLastSnapshot;

	static constexpr float RecentMontageTimeout = 0.5;

	inline static bool bServerAnimRateWasSet = false;

	float TimeBetweenServerAnimEvaluations;

	uint8 IndexOfOldestSnapshot = 0;

	FPoseSnapshot CurrentSnapshot;
};
