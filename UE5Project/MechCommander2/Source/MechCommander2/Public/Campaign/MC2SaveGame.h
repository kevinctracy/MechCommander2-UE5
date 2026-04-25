#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MC2SaveGame.generated.h"

/**
 * FMC2PilotRecord
 * Mirrors MC2's Pilot struct from pilot.h.
 * Tracks XP, kills, and which lance the pilot is in.
 */
USTRUCT(BlueprintType)
struct FMC2PilotRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FName PilotID;               // DT_Pilots row key

	UPROPERTY(BlueprintReadWrite)
	int32 ExperiencePoints = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 Kills = 0;

	UPROPERTY(BlueprintReadWrite)
	bool bAlive = true;

	// Current skill values (copied from DT_Pilots on recruit; improved via XP)
	// Lower = better (BattleTech standard). Range 2-8.
	UPROPERTY(BlueprintReadWrite)
	int32 Gunnery  = 4;

	UPROPERTY(BlueprintReadWrite)
	int32 Piloting = 5;
};

/**
 * FMC2MechRecord
 * Represents a mech in the player's inventory with its current loadout.
 */
USTRUCT(BlueprintType)
struct FMC2MechRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FName ChassisID;             // DT_MechChassis row key

	UPROPERTY(BlueprintReadWrite)
	int32 VariantIndex = 0;      // DT_MechVariants index

	// Component loadout: slot index → DT_Components row key (empty = empty slot)
	UPROPERTY(BlueprintReadWrite)
	TArray<FName> ComponentSlots;

	UPROPERTY(BlueprintReadWrite)
	FName AssignedPilotID;       // empty = unassigned

	// Per-zone current armor (11 zones: Head,LA,RA,LT,RT,CT,LL,RL,RearLT,RearRT,RearCT)
	// Empty array = fully repaired / fresh from factory.
	UPROPERTY(BlueprintReadWrite)
	TArray<int32> CurrentArmor;
};

/**
 * FMC2MissionResult
 * Saved outcome for a completed mission.
 */
USTRUCT(BlueprintType)
struct FMC2SavedMissionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FName MissionID;

	UPROPERTY(BlueprintReadWrite)
	uint8 ResultValue = 0;       // cast of EMC2MissionResult

	UPROPERTY(BlueprintReadWrite)
	int32 CBillsEarned = 0;
};

/**
 * UMC2SaveGame
 * Serializes the full campaign state to disk.
 * Mirrors MC2's logisticsData / campaignData persistence system.
 *
 * Slot name convention: "MC2_Save_0", "MC2_Save_1", etc.
 * User index: always 0 (single-player).
 */
UCLASS()
class MECHCOMMANDER2_API UMC2SaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// --- Campaign identity ---

	UPROPERTY(BlueprintReadWrite)
	FString CommanderName;

	UPROPERTY(BlueprintReadWrite)
	int32 CurrentOperationIndex = 0;   // which operation the player is on

	UPROPERTY(BlueprintReadWrite)
	int32 CurrentMissionIndex   = 0;   // which mission within the operation

	// --- Resources ---

	UPROPERTY(BlueprintReadWrite)
	int32 CBills = 1000000;            // C-Bills (currency), default = 1M

	UPROPERTY(BlueprintReadWrite)
	int32 ResourcePoints = 0;         // RP for purchasing support

	// --- Roster ---

	UPROPERTY(BlueprintReadWrite)
	TArray<FMC2MechRecord> MechRoster;

	UPROPERTY(BlueprintReadWrite)
	TArray<FMC2PilotRecord> PilotRoster;

	// Component inventory (salvaged components not yet installed on a mech)
	// Each entry is a DT_Components row key.
	UPROPERTY(BlueprintReadWrite)
	TArray<FName> ComponentInventory;

	// --- Mission history ---

	UPROPERTY(BlueprintReadWrite)
	TArray<FMC2SavedMissionResult> CompletedMissions;

	// --- Eternal ABL flags preserved across missions ---
	// 64 booleans matching AMC2GameState::MissionFlags semantics for campaign globals.

	UPROPERTY(BlueprintReadWrite)
	TArray<bool> CampaignFlags;        // length 64

	// --- Save metadata ---

	UPROPERTY(BlueprintReadWrite)
	FDateTime SaveTimestamp;

	UPROPERTY(BlueprintReadWrite)
	float TotalPlaytimeSeconds = 0.f;

	UMC2SaveGame()
	{
		CampaignFlags.Init(false, 64);
	}
};
