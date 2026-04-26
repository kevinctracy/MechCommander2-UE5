#include "AI/MC2NavModifierVolume.h"
#include "AI/MC2NavAreas.h"

AMC2NavModifierVolume::AMC2NavModifierVolume()
{
	ApplyTerrainAreaClass();
}

void AMC2NavModifierVolume::ApplyTerrainAreaClass()
{
	switch (TerrainType)
	{
	case EMC2TerrainType::Water:
		AreaClass = UMC2NavArea_Water::StaticClass();
		break;
	case EMC2TerrainType::ShallowWater:
		AreaClass = UMC2NavArea_ShallowWater::StaticClass();
		break;
	case EMC2TerrainType::Cliff:
		AreaClass = UMC2NavArea_Cliff::StaticClass();
		break;
	case EMC2TerrainType::HeavyTerrain:
		AreaClass = UMC2NavArea_HeavyTerrain::StaticClass();
		break;
	}
}

#if WITH_EDITOR
void AMC2NavModifierVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(AMC2NavModifierVolume, TerrainType))
		ApplyTerrainAreaClass();
}
#endif
