#include "AI/MC2NavQueryFilter.h"
#include "NavigationSystem.h"

UMC2NavQueryFilter_Light::UMC2NavQueryFilter_Light()
{
	// Light mechs use default traversal costs; no restrictions
	SetMaxSearchNodes(2048);
}

UMC2NavQueryFilter_Medium::UMC2NavQueryFilter_Medium()
{
	SetMaxSearchNodes(2048);
	// Medium mechs pay 2x through HeavyOnly areas (NavArea_HeavyRestricted)
	// Area classes and costs are set in the UE editor via the filter details panel
}

UMC2NavQueryFilter_Heavy::UMC2NavQueryFilter_Heavy()
{
	SetMaxSearchNodes(2048);
	// Heavy/Assault mechs pay 3x through restricted areas; completely blocked from NarrowPass areas
}
