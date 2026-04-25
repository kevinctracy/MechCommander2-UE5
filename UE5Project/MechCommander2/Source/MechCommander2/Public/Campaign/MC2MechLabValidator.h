#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MC2DataTableRows.h"
#include "Campaign/MC2SaveGame.h"
#include "MC2MechLabValidator.generated.h"

UENUM(BlueprintType)
enum class EMC2SlotLocation : uint8
{
	Head       UMETA(DisplayName = "Head"),
	CenterTorso UMETA(DisplayName = "Center Torso"),
	LeftTorso   UMETA(DisplayName = "Left Torso"),
	RightTorso  UMETA(DisplayName = "Right Torso"),
	LeftArm     UMETA(DisplayName = "Left Arm"),
	RightArm    UMETA(DisplayName = "Right Arm"),
	LeftLeg     UMETA(DisplayName = "Left Leg"),
	RightLeg    UMETA(DisplayName = "Right Leg"),
};

USTRUCT(BlueprintType)
struct FMC2ValidationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bIsValid = true;

	UPROPERTY(BlueprintReadOnly)
	FString ErrorMessage;

	// Weight breakdown
	UPROPERTY(BlueprintReadOnly)
	float TotalWeight = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float MaxWeight = 0.f;

	// Slot usage per location (index = EMC2SlotLocation)
	UPROPERTY(BlueprintReadOnly)
	TArray<int32> UsedSlots;

	UPROPERTY(BlueprintReadOnly)
	TArray<int32> MaxSlots;
};

/**
 * UMC2MechLabValidator
 * Pure-function utility for validating a mech loadout against chassis constraints.
 * Called by WBP_MechLab drag-drop operations to prevent invalid configurations.
 *
 * Slot capacity per location comes from FMC2MechChassisRow (Data Table).
 * Component slot cost and weight come from FMC2ComponentRow.
 *
 * Fixed slot capacities (BattleTech standard Inner Sphere):
 *   Head: 1 slot, CT: 12, LT/RT: 12, LA/RA: 12, LL/RL: 6
 *   (actual values come from DT_MechChassis per-chassis, not hardcoded)
 */
UCLASS(BlueprintType, Blueprintable)
class MECHCOMMANDER2_API UMC2MechLabValidator : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Validate a full mech loadout.
	 * @param ChassisRow      The chassis data (tonnage, slot counts per location)
	 * @param ComponentSlots  Array of 20 component MasterIDs (matching FMC2MechRecord::ComponentSlots)
	 * @param ComponentTable  Data Table containing FMC2ComponentRow rows
	 * @return                Validation result with weight, slot usage, and error info
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MechLab")
	static FMC2ValidationResult ValidateLoadout(
		const FMC2MechChassisRow& ChassisRow,
		const TArray<FName>& ComponentSlots,
		UDataTable* ComponentTable
	);

	/**
	 * Check whether a single component can be added to a specific location.
	 * @param ComponentRow    The component to add
	 * @param Location        Where the player is trying to drop it
	 * @param CurrentSlots    Current slot usage for all locations
	 * @param ChassisRow      Chassis constraints
	 * @return                True if the component fits in that location
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MechLab")
	static bool CanAddComponentToLocation(
		const FMC2ComponentRow& ComponentRow,
		EMC2SlotLocation Location,
		const TArray<int32>& CurrentSlots,
		const FMC2MechChassisRow& ChassisRow
	);

	/**
	 * Calculate total component weight for a loadout.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MechLab")
	static float CalculateTotalWeight(
		const TArray<FName>& ComponentSlots,
		UDataTable* ComponentTable
	);

	/**
	 * Get slot capacity for a given body location.
	 * All MC2 IS mechs use standard BT slot counts: Head=1, CT/LT/RT/LA/RA=12, LL/RL=6.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MechLab")
	static int32 GetLocationMaxSlots(const FMC2MechChassisRow& ChassisRow, EMC2SlotLocation Location);

	/**
	 * Check if a component type is allowed in a given body location.
	 * Uses FMC2ComponentRow bool fields: LocHead, LocCT, LocLT, LocRT, LocLA, LocRA, LocLL, LocRL.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MechLab")
	static bool IsComponentAllowedInLocation(const FMC2ComponentRow& ComponentRow, EMC2SlotLocation Location);

	/**
	 * Generate a FMC2MechRecord for a validated loadout, ready to save.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MechLab")
	static FMC2MechRecord BuildMechRecord(
		FName ChassisID,
		int32 VariantIndex,
		const TArray<FName>& ComponentSlots
	);

	/**
	 * Check whether a component is available to the player.
	 * Clan-tech components require bClanTechUnlocked = true (set when campaign reaches operation 3).
	 * IS and Both components are always available.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MechLab|TechBase")
	static bool IsComponentAvailable(const FMC2ComponentRow& ComponentRow, bool bClanTechUnlocked);

	/**
	 * Filter a component list down to those the player can currently use.
	 * Returns row names (FNames) that pass the tech-base availability check.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MechLab|TechBase")
	static TArray<FName> FilterAvailableComponents(UDataTable* ComponentTable, bool bClanTechUnlocked);

private:
	// Slot counts for standard Inner Sphere mechs (fallback if chassis data missing)
	static const int32 DefaultSlots[];
};
