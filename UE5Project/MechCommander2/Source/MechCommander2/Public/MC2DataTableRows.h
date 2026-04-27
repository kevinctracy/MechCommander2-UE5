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
 *   DT_Components       — FMC2ComponentRow        (from compbas.csv)
 *   DT_MechChassis      — FMC2MechChassisRow      (from atlas.csv, madcat.csv, ...)
 *   DT_MechVariants     — FMC2MechVariantRow      (variant loadouts per chassis)
 *   DT_VehicleTypes     — FMC2VehicleTypeRow      (from *.fit vehicle files)
 *   DT_BuildingTypes    — FMC2BuildingTypeRow     (from *.fit building/prop files)
 *   DT_Pilots           — FMC2PilotRow            (from Pilots.csv)
 *   DT_CampaignGroups   — FMC2CampaignGroupRow    (from campaign.fit — group metadata)
 *   DT_CampaignMissions — FMC2CampaignMissionRow  (from campaign.fit — per-mission rows)
 *   DT_TutorialGroups   — FMC2CampaignGroupRow    (from tutorial.fit)
 *   DT_TutorialMissions — FMC2CampaignMissionRow  (from tutorial.fit)
 *   DT_CameraSettings   — FMC2CameraSettingsRow   (from Cameras/Cameras.fit)
 *   DT_TeamColors       — FMC2TeamColorRow        (from Cameras/Colors.fit)
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

// ---------------------------------------------------------------------------
// DT_CampaignGroups / DT_TutorialGroups
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FMC2CampaignGroupRow : public FTableRowBase
{
	GENERATED_BODY()

	// "Campaign" or "Tutorial"
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString CampaignID        = {};
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   GroupIndex         = 0;
	// How many missions the player must complete before this group is done.
	// If NumberToComplete < MissionCount the player chooses which to run.
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   NumberToComplete   = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   MissionCount       = 1;
	// Localisation key for the operation briefing widget
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString OperationFile      = {};
	// Node video identifier played when this group is unlocked on the galaxy map
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Video              = {};
	// Optional cinema cutscene shown before the group's briefing
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString PreVideo           = {};
	// Music track index (matches MC2 music table)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   Tune               = 0;
	// ABL script run at logistics screen entry for this group
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString ABLScript          = {};
};

// ---------------------------------------------------------------------------
// DT_CampaignMissions / DT_TutorialMissions
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FMC2CampaignMissionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString CampaignID          = {};
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   GroupIndex           = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   MissionIndex         = 0;
	// Level filename without extension (e.g. "mc2_01", "tut_01")
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString FileName             = {};
	// If true this mission must be completed; false means it's one of N choices
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool    Mandatory            = true;
	// Logistics purchase file loaded before this mission
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString PurchaseFile         = {};
	// Which post-mission screens to show
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool    PlayLogistics        = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool    PlaySalvage          = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool    PlayPilotPromotion   = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool    PlayPurchasing       = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool    PlaySelection        = false;
	// Hidden missions are not shown on the galaxy map until triggered
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool    Hidden               = false;
	// If set, overrides the parent group's Video with this specific node video
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString VideoOverride        = {};
};

// ---------------------------------------------------------------------------
// DT_CameraSettings  (single-row table — row name "Default")
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FMC2CameraSettingsRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   ProjectionAngle        = 35.f;   // degrees
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   PositionX              = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   PositionY              = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   PositionZ              = 0.f;
	// Directional light colour (0-255 per channel)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   LightRed               = 255;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   LightGreen             = 255;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   LightBlue              = 255;
	// Ambient light colour
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   AmbientRed             = 31;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   AmbientGreen           = 31;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   AmbientBlue            = 31;
	// "Seen" fog-of-war tile tint
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   SeenRed                = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   SeenGreen              = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   SeenBlue               = 255;
	// Base terrain colour modifier
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   BaseRed                = 47;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   BaseGreen              = 47;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   BaseBlue               = 47;
	// Light direction
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   LightDirPitch          = 22.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   LightDirYaw            = -61.25f;
	// Default camera zoom and scale
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   NewScale               = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   StartRotation          = 180.f;
	// LOD distance scale factors for three LOD levels
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   LODScale0              = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   LODScale1              = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   LODScale2              = 0.3f;
	// Terrain elevation adjustment
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   ElevationAdjustFactor  = 150.f;
	// Zoom limits
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   ZoomMax                = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   ZoomMin                = 0.1f;
	// Fog
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   FogStart               = 800.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float   FogFull                = 250.f;
	// ARGB hex string e.g. "#FFA0A0A5"
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString FogColor               = TEXT("#FFA0A0A5");
};

// ---------------------------------------------------------------------------
// DT_TeamColors
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FMC2TeamColorRow : public FTableRowBase
{
	GENERATED_BODY()

	// Which palette (0 = primary team colors, 1 = neutral/grey tones)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   TableIndex  = 0;
	// Index within the table (0-55)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   ColorIndex  = 0;
	// ARGB hex string e.g. "#FF001000"
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString ARGB        = {};
	// Individual channels (0-255)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   A           = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   R           = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   G           = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32   B           = 0;
};
