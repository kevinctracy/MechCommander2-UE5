#pragma once

#include "CoreMinimal.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "MC2NavQueryFilter.generated.h"

/**
 * UMC2NavQueryFilter_Light
 * Light mechs (< 40t) — default agent; can traverse all passable areas.
 */
UCLASS()
class MECHCOMMANDER2_API UMC2NavQueryFilter_Light : public UNavigationQueryFilter
{
	GENERATED_BODY()
public:
	UMC2NavQueryFilter_Light();
};

/**
 * UMC2NavQueryFilter_Medium
 * Medium mechs (40-55t) — avoid terrain marked as HeavyOnly (cliffs, dense forest).
 */
UCLASS()
class MECHCOMMANDER2_API UMC2NavQueryFilter_Medium : public UNavigationQueryFilter
{
	GENERATED_BODY()
public:
	UMC2NavQueryFilter_Medium();
};

/**
 * UMC2NavQueryFilter_Heavy
 * Heavy/Assault mechs (60t+) — higher cost through soft terrain; blocked from narrow corridors.
 * NavModifierVolumes add cost (HeavyTerrainCost=3.0) to areas marked for weight restriction.
 */
UCLASS()
class MECHCOMMANDER2_API UMC2NavQueryFilter_Heavy : public UNavigationQueryFilter
{
	GENERATED_BODY()
public:
	UMC2NavQueryFilter_Heavy();
};
