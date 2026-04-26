#pragma once

#include "CoreMinimal.h"
#include "NavMesh/NavModifierVolume.h"
#include "MC2NavModifierVolume.generated.h"

UENUM(BlueprintType)
enum class EMC2TerrainType : uint8
{
	Water         UMETA(DisplayName = "Deep Water (blocked for heavy mechs)"),
	ShallowWater  UMETA(DisplayName = "Shallow Water / Mud"),
	Cliff         UMETA(DisplayName = "Cliff / Impassable"),
	HeavyTerrain  UMETA(DisplayName = "Dense Forest / Rubble"),
};

/**
 * AMC2NavModifierVolume
 * Convenience subclass of ANavModifierVolume that auto-assigns the correct
 * MC2 nav area class based on terrain type selection in the editor.
 *
 * Place over water, cliffs, and dense forest on the Landscape.
 * The area class feeds into UMC2NavQueryFilter_Heavy to block heavy mechs from
 * water/cliff zones while still letting light mechs pass at higher cost.
 *
 * Workflow:
 *   1. Place volume over terrain feature
 *   2. Set TerrainType in Details panel
 *   3. Area class is auto-set; rebuild nav mesh
 */
UCLASS(BlueprintType, Blueprintable)
class MECHCOMMANDER2_API AMC2NavModifierVolume : public ANavModifierVolume
{
	GENERATED_BODY()

public:
	AMC2NavModifierVolume();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MC2|Terrain")
	EMC2TerrainType TerrainType = EMC2TerrainType::Water;

#if WITH_EDITOR
	// Auto-assign area class when terrain type changes in editor
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void ApplyTerrainAreaClass();
};
