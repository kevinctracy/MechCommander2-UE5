#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MC2Objective.generated.h"

class AMC2Mover;
class AMC2GameMode;
class ATriggerVolume;

UENUM(BlueprintType)
enum class EMC2ObjectiveStatus : uint8
{
	Incomplete  UMETA(DisplayName = "Incomplete"),
	Complete    UMETA(DisplayName = "Complete"),
	Failed      UMETA(DisplayName = "Failed"),
};

/**
 * UMC2Objective
 * Abstract base for all 23 MC2 objective condition types from Objective.h.
 * Each subclass implements Evaluate() which returns the current status.
 * Objectives are polled every frame by AMC2GameMode.
 *
 * Instances are created as UPROPERTY(Instanced) on AMC2GameMode so they
 * can be configured directly in the Level Blueprint or placed GameMode actor.
 */
UCLASS(Abstract, EditInlineNew, BlueprintType)
class MECHCOMMANDER2_API UMC2Objective : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Objective")
	EMC2ObjectiveStatus Status = EMC2ObjectiveStatus::Incomplete;

	UFUNCTION(BlueprintCallable, Category = "Objective")
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) PURE_VIRTUAL(UMC2Objective::Evaluate, return EMC2ObjectiveStatus::Incomplete;);
};

// ---------------------------------------------------------------------------
// DESTROY group (5 conditions)
// ---------------------------------------------------------------------------

UCLASS(meta=(DisplayName="Destroy All Enemy Units"))
class MECHCOMMANDER2_API UObj_DestroyAllEnemy : public UMC2Objective
{
	GENERATED_BODY()
public:
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

UCLASS(meta=(DisplayName="Destroy N Enemy Units"))
class MECHCOMMANDER2_API UObj_DestroyNumberOfEnemy : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	int32 RequiredCount = 5;

	int32 DestroyedCount = 0;
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

UCLASS(meta=(DisplayName="Destroy Enemy Unit Group"))
class MECHCOMMANDER2_API UObj_DestroyEnemyGroup : public UMC2Objective
{
	GENERATED_BODY()
public:
	// Tag name shared by all units in this group (set on their actor via Tags array)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FName GroupTag;

	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

UCLASS(meta=(DisplayName="Destroy Specific Enemy Unit"))
class MECHCOMMANDER2_API UObj_DestroySpecificEnemy : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	TWeakObjectPtr<AMC2Mover> TargetUnit;

	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

UCLASS(meta=(DisplayName="Destroy Specific Structure"))
class MECHCOMMANDER2_API UObj_DestroySpecificStructure : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	TWeakObjectPtr<AActor> TargetStructure;

	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

// ---------------------------------------------------------------------------
// CAPTURE_OR_DESTROY group (5 conditions)
// ---------------------------------------------------------------------------

UCLASS(meta=(DisplayName="Capture or Destroy All Enemies"))
class MECHCOMMANDER2_API UObj_CaptureOrDestroyAllEnemy : public UMC2Objective
{
	GENERATED_BODY()
public:
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

UCLASS(meta=(DisplayName="Capture or Destroy N Enemies"))
class MECHCOMMANDER2_API UObj_CaptureOrDestroyNumber : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	int32 RequiredCount = 3;
	int32 CountAchieved = 0;
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

UCLASS(meta=(DisplayName="Capture or Destroy Enemy Group"))
class MECHCOMMANDER2_API UObj_CaptureOrDestroyGroup : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FName GroupTag;
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

UCLASS(meta=(DisplayName="Capture or Destroy Specific Enemy"))
class MECHCOMMANDER2_API UObj_CaptureOrDestroySpecificEnemy : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	TWeakObjectPtr<AMC2Mover> TargetUnit;
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

UCLASS(meta=(DisplayName="Capture or Destroy Specific Structure"))
class MECHCOMMANDER2_API UObj_CaptureOrDestroySpecificStructure : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	TWeakObjectPtr<AActor> TargetStructure;
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

// ---------------------------------------------------------------------------
// DEAD_OR_FLED group (4 conditions)
// ---------------------------------------------------------------------------

UCLASS(meta=(DisplayName="Dead or Fled All Enemies"))
class MECHCOMMANDER2_API UObj_DeadOrFledAllEnemy : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	float MapBoundaryRadius = 15000.f;
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

UCLASS(meta=(DisplayName="Dead or Fled N Enemies"))
class MECHCOMMANDER2_API UObj_DeadOrFledNumber : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	int32 RequiredCount = 3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	float MapBoundaryRadius = 15000.f;
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

UCLASS(meta=(DisplayName="Dead or Fled Enemy Group"))
class MECHCOMMANDER2_API UObj_DeadOrFledGroup : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FName GroupTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	float MapBoundaryRadius = 15000.f;
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

UCLASS(meta=(DisplayName="Dead or Fled Specific Enemy"))
class MECHCOMMANDER2_API UObj_DeadOrFledSpecific : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	TWeakObjectPtr<AMC2Mover> TargetUnit;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	float MapBoundaryRadius = 15000.f;
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

// ---------------------------------------------------------------------------
// CAPTURE group (2 conditions)
// ---------------------------------------------------------------------------

UCLASS(meta=(DisplayName="Capture Unit"))
class MECHCOMMANDER2_API UObj_CaptureUnit : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	TWeakObjectPtr<AMC2Mover> TargetUnit;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	float CaptureRadius = 300.f;
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

UCLASS(meta=(DisplayName="Capture Structure"))
class MECHCOMMANDER2_API UObj_CaptureStructure : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	TWeakObjectPtr<AActor> TargetStructure;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	float CaptureRadius = 400.f;
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

// ---------------------------------------------------------------------------
// GUARD group (2 conditions)
// ---------------------------------------------------------------------------

UCLASS(meta=(DisplayName="Guard Specific Unit"))
class MECHCOMMANDER2_API UObj_GuardUnit : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	TWeakObjectPtr<AMC2Mover> GuardedUnit;
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

UCLASS(meta=(DisplayName="Guard Specific Structure"))
class MECHCOMMANDER2_API UObj_GuardStructure : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	TWeakObjectPtr<AActor> GuardedStructure;
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

// ---------------------------------------------------------------------------
// MOVE group (4 conditions)
// ---------------------------------------------------------------------------

UCLASS(meta=(DisplayName="Move Any Unit to Area"))
class MECHCOMMANDER2_API UObj_MoveAnyUnitToArea : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	TWeakObjectPtr<ATriggerVolume> TriggerArea;
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

UCLASS(meta=(DisplayName="Move All Units to Area"))
class MECHCOMMANDER2_API UObj_MoveAllUnitsToArea : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	TWeakObjectPtr<ATriggerVolume> TriggerArea;
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

UCLASS(meta=(DisplayName="Move All Surviving Units to Area"))
class MECHCOMMANDER2_API UObj_MoveAllSurvivingUnitsToArea : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	TWeakObjectPtr<ATriggerVolume> TriggerArea;
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

UCLASS(meta=(DisplayName="Move All Surviving Mechs to Area"))
class MECHCOMMANDER2_API UObj_MoveAllSurvivingMechsToArea : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	TWeakObjectPtr<ATriggerVolume> TriggerArea;
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

// ---------------------------------------------------------------------------
// STATE conditions (2 conditions)
// ---------------------------------------------------------------------------

UCLASS(meta=(DisplayName="Boolean Flag Is Set"))
class MECHCOMMANDER2_API UObj_BooleanFlagIsSet : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	int32 FlagIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	bool RequiredValue = true;
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};

UCLASS(meta=(DisplayName="Elapsed Mission Time"))
class MECHCOMMANDER2_API UObj_ElapsedMissionTime : public UMC2Objective
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	int32 TimerIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	float RequiredSeconds = 300.f;
	virtual EMC2ObjectiveStatus Evaluate(AMC2GameMode* GameMode) override;
};
