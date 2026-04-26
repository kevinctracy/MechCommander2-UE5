#pragma once

#include "CoreMinimal.h"
#include "NavAreas/NavArea.h"
#include "NavAreas/NavArea_Obstacle.h"
#include "MC2NavAreas.generated.h"

/**
 * UMC2NavArea_Water
 * Marks water terrain on the Landscape.
 * Light mechs: high cost (avoid unless ordered). Heavy mechs: blocked entirely via UMC2NavQueryFilter_Heavy.
 * Shallow water (< 0.5m) gets NavArea_Default cost; deep water uses this class.
 */
UCLASS()
class MECHCOMMANDER2_API UMC2NavArea_Water : public UNavArea
{
	GENERATED_BODY()
public:
	UMC2NavArea_Water()
	{
		DefaultCost      = 4.f;   // 4× normal cost — mechs really don't like water
		FixedAreaEnteringCost = 0.f;
		// UMC2NavQueryFilter_Heavy will set this area as blocked (cost = BIG_NUMBER)
		DrawColor = FColor(0, 100, 200);  // blue in nav mesh debug view
	}
};

/**
 * UMC2NavArea_Cliff
 * Marks impassable cliff faces and extreme slopes (>45°).
 * All mechs are blocked — placed on sheer terrain using NavModifierVolumes.
 */
UCLASS()
class MECHCOMMANDER2_API UMC2NavArea_Cliff : public UNavArea_Obstacle
{
	GENERATED_BODY()
public:
	UMC2NavArea_Cliff()
	{
		DrawColor = FColor(180, 100, 0);  // orange in nav mesh debug view
	}
};

/**
 * UMC2NavArea_ShallowWater
 * Shallow water / mud — slight cost penalty, passable for all mechs.
 * Used for swamp/shore terrain visible in missions 3, 14, 20.
 */
UCLASS()
class MECHCOMMANDER2_API UMC2NavArea_ShallowWater : public UNavArea
{
	GENERATED_BODY()
public:
	UMC2NavArea_ShallowWater()
	{
		DefaultCost = 1.8f;   // slight penalty — passable but not preferred
		DrawColor   = FColor(0, 160, 160);
	}
};

/**
 * UMC2NavArea_HeavyTerrain
 * Dense forest, rubble, craters — higher cost for heavy/assault mechs.
 * Referenced by UMC2NavQueryFilter_Heavy (cost multiplied to 3.0).
 */
UCLASS()
class MECHCOMMANDER2_API UMC2NavArea_HeavyTerrain : public UNavArea
{
	GENERATED_BODY()
public:
	UMC2NavArea_HeavyTerrain()
	{
		DefaultCost = 2.f;
		DrawColor   = FColor(100, 80, 40);  // brown
	}
};
