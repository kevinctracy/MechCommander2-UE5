#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MC2GameState.h"
#include "Mission/MC2Objective.h"
#include "MC2GameMode.generated.h"

class AMC2Mover;
class UMC2Objective;

/**
 * FMC2Part
 * Defines a unit to spawn at mission start.
 * Matches MC2's Part struct from mission.h:
 *   objNumber -> UnitClassID (Data Table row key)
 *   position  -> SpawnTransform
 *   teamIndex -> TeamIndex
 */
USTRUCT(BlueprintType)
struct FMC2Part
{
	GENERATED_BODY()

	// Row key into DT_MechChassis or DT_VehicleTypes
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName UnitClassID;

	// Subclass to spawn (set from Data Table at runtime)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AMC2Mover> UnitClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform SpawnTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TeamIndex = 1;

	// Variant index into DT_MechVariants
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 VariantIndex = 0;

	// Pilot name (DT_Pilots row key), empty = generic AI
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName PilotID;
};

/**
 * AMC2GameMode
 * Manages the mission lifecycle:
 *   - Spawns all units from the Parts array at mission start
 *   - Evaluates all Objectives every tick
 *   - Triggers win/lose when objectives are decided
 *   - Provides ABL-equivalent API for Blueprint mission scripts
 *
 * Maps directly to MC2's Mission class (mission.h/mission.cpp).
 */
UCLASS()
class MECHCOMMANDER2_API AMC2GameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AMC2GameMode();

	// --- Parts (unit spawn definitions) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Parts")
	TArray<FMC2Part> Parts;

	// --- Objectives ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Mission|Objectives")
	TArray<TObjectPtr<UMC2Objective>> PrimaryObjectives;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Mission|Objectives")
	TArray<TObjectPtr<UMC2Objective>> SecondaryObjectives;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Mission|Objectives")
	TArray<TObjectPtr<UMC2Objective>> BonusObjectives;

	// --- Mission config ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Mission")
	float MissionTimeLimit = 0.f;   // 0 = no time limit

	// --- ABL-equivalent mission script API (callable from Blueprint) ---

	UFUNCTION(BlueprintCallable, Category = "Mission|Script")
	void ScriptSetMissionFlag(int32 FlagIndex, bool Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission|Script")
	bool ScriptGetMissionFlag(int32 FlagIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Mission|Script")
	void ScriptStartTimer(int32 TimerIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission|Script")
	float ScriptGetTimer(int32 TimerIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Mission|Script")
	AMC2Mover* ScriptSpawnUnit(TSubclassOf<AMC2Mover> UnitClass, const FVector& Location, int32 Team);

	UFUNCTION(BlueprintCallable, Category = "Mission|Script")
	void ScriptSetMissionResult(EMC2MissionResult Result);

	// --- Spawned unit tracking ---

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission")
	const TArray<AMC2Mover*>& GetAllUnits() const { return AllSpawnedUnits; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission")
	TArray<AMC2Mover*> GetUnitsByTeam(int32 TeamIndex) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission")
	bool IsTeamDefeated(int32 TeamIndex) const;

	// --- Events (implement in Blueprint per mission) ---

	UFUNCTION(BlueprintImplementableEvent, Category = "Mission")
	void OnMissionStart();

	UFUNCTION(BlueprintImplementableEvent, Category = "Mission")
	void OnMissionResult(EMC2MissionResult Result);

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	void SpawnAllParts();
	void EvaluateObjectives();
	void ApplyMissionResult(EMC2MissionResult Result);
	void CommitMissionToSave(EMC2MissionResult Result);  // autosaves, commits pilot XP

	TArray<AMC2Mover*> AllSpawnedUnits;
	bool bMissionOver = false;
};
