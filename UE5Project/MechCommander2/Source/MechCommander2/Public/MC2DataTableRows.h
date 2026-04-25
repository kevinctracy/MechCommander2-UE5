#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MC2DataTableRows.generated.h"

/**
 * Tech base: Inner Sphere equipment is heavier/bulkier but available from the campaign start.
 * Clan equipment is lighter/more compact but locked behind campaign progression (CampaignFlag 32+).
 * Matches MC2's original compbas.csv TechBase column values.
 */
UENUM(BlueprintType)
enum class EMC2TechBase : uint8
{
	InnerSphere UMETA(DisplayName = "Inner Sphere"),
	Clan        UMETA(DisplayName = "Clan"),
	Both        UMETA(DisplayName = "Both / Mixed"),
};

/**
 * Row structs for UE5 Data Tables imported from fit_convert.py CSV output.
 * Property names must match CSV column headers exactly (case-sensitive).
 * Import each CSV in UE5 via: Content Browser → Import → pick CSV → select row struct.
 *
 * Data Tables:
 *   DT_Components    — FMC2ComponentRow     (from compbas.csv)
 *   DT_MechChassis   — FMC2MechChassisRow   (from atlas.csv, madcat.csv, ...)
 *   DT_MechVariants  — FMC2MechVariantRow   (variant loadouts per chassis)
 *   DT_VehicleTypes  — FMC2VehicleTypeRow   (from *.fit vehicle files)
 *   DT_BuildingTypes — FMC2BuildingTypeRow  (from *.fit building/prop files)
 *   DT_Pilots        — FMC2PilotRow         (from Pilots.csv)
 */

// ---------------------------------------------------------------------------
// DT_Components
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FMC2ComponentRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   MasterID          = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Type              = {};
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Name              = {};
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   CritHits          = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   RecycleTime       = 0.f;  // seconds
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   Heat              = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   Weight            = 0.f;  // tons
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   Damage            = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   BattleRating      = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   RefitPoints       = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   Range             = 0.f;  // game units

	// Mountable locations (True = can mount here)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool    LocHead           = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool    LocCT             = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool    LocLT             = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool    LocRT             = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool    LocLA             = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool    LocRA             = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool    LocLL             = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool    LocRL             = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString MissileType       = {};
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   SpecialFlags      = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   FXIndex           = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   AmmoMasterID      = 0;   // 0 = no ammo needed
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString LogisticsIcon1    = {};
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString LogisticsIcon2    = {};
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   StringIndex       = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   EncyclopediaIndex = 0;

	// Tech base — controls availability in MechLab. Clan gear requires campaign unlock.
	// CSV column "TechBase": 0=IS, 1=Clan, 2=Both
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EMC2TechBase TechBase = EMC2TechBase::InnerSphere;
};

// ---------------------------------------------------------------------------
// DT_MechChassis
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FMC2MechChassisRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString MechName        = {};
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   Tonnage         = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   HeatIndex       = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   LoadIndex       = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   TotalArmor      = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   MaxRunSpeed     = 0.f;  // km/h → converted to UU/s at runtime
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   TorsoYawRate    = 360;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   MaxTorsoYaw     = 120;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   ChassisBR       = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   ChassisC_Bills  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   MechParts       = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   MaxArmor        = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   DescIndex       = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   AbbrIndex       = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   EncyclopediaID  = -1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   HelpId          = -1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString AnimationName   = {};
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString LittleIcon      = {};
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString BigIcon         = {};

	// Internal structure per section
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   IS_Head         = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   IS_LeftArm      = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   IS_RightArm     = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   IS_LeftTorso    = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   IS_RightTorso   = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   IS_CenterTorso  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   IS_LeftLeg      = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   IS_RightLeg     = 0;

	// Armor points per section
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   Armor_Head      = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   Armor_LA        = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   Armor_RA        = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   Armor_LT        = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   Armor_RT        = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   Armor_CT        = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   Armor_LL        = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   Armor_RL        = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   Armor_RearLT    = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   Armor_RearRT    = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   Armor_RearCT    = 0;
};

// ---------------------------------------------------------------------------
// DT_MechVariants
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FMC2MechVariantRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString MechName    = {};
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString VariantID   = {};
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString VariantName = {};

	// Component MasterIDs in each slot (0 = empty)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item0  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item1  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item2  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item3  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item4  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item5  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item6  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item7  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item8  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item9  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item10 = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item11 = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item12 = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item13 = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item14 = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item15 = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item16 = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item17 = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item18 = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Item19 = 0;
};

// ---------------------------------------------------------------------------
// DT_VehicleTypes
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FMC2VehicleTypeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Name              = {};
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString AppearanceName    = {};
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   ObjectTypeNum     = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   ExplosionObject   = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   DestroyedObject   = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   ExtentRadius      = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   MaxVelocity       = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   MaxAccel          = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   MaxTurretYawRate  = 360;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   MaxTurretYaw      = 360;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   MaxVehicleYawRate = 360;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   Tonnage           = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   BattleRating      = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   IconIndex         = 0;

	// Armor — 5 vehicle arcs (matches MC2's vehicle hit arcs)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Armor_Front  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Armor_Rear   = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Armor_Left   = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Armor_Right  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Armor_Turret = 0;

	// Internal structure
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 IS_Front  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 IS_Rear   = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 IS_Left   = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 IS_Right  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 IS_Turret = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 NumWeapons = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 NumAmmo    = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 NumOther   = 0;
};

// ---------------------------------------------------------------------------
// DT_BuildingTypes
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FMC2BuildingTypeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Name            = {};
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString AppearanceName  = {};
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   ObjectTypeNum   = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   ExplosionObject = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   DestroyedObject = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   ExtentRadius    = 0.f;
};

// ---------------------------------------------------------------------------
// DT_Pilots
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FMC2PilotRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString PilotName       = {};
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Rank            = {};
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   Gunnery         = 4;   // lower = better (BattleTech standard)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   Piloting        = 5;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   InitiativePips  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   HouseID         = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString PortraitFile    = {};
};
