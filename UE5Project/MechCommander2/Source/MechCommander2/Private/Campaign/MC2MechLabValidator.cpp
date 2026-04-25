#include "Campaign/MC2MechLabValidator.h"
#include "Engine/DataTable.h"

// BT standard IS mech slot capacities: Head, CT, LT, RT, LA, RA, LL, RL
const int32 UMC2MechLabValidator::DefaultSlots[] = { 1, 12, 12, 12, 12, 12, 6, 6 };

// ---------------------------------------------------------------------------

FMC2ValidationResult UMC2MechLabValidator::ValidateLoadout(
	const FMC2MechChassisRow& ChassisRow,
	const TArray<FName>& ComponentSlots,
	UDataTable* ComponentTable)
{
	FMC2ValidationResult Result;
	const int32 NumLocations = 8;
	Result.UsedSlots.Init(0, NumLocations);
	Result.MaxSlots.Init(0, NumLocations);

	// Fill max slots from chassis row
	for (int32 i = 0; i < NumLocations; ++i)
		Result.MaxSlots[i] = GetLocationMaxSlots(ChassisRow, (EMC2SlotLocation)i);

	Result.MaxWeight = (float)ChassisRow.Tonnage;
	Result.TotalWeight = 0.f;

	if (!ComponentTable)
	{
		Result.bIsValid = false;
		Result.ErrorMessage = TEXT("Component data table not set.");
		return Result;
	}

	for (const FName& CompID : ComponentSlots)
	{
		if (CompID.IsNone())
			continue;

		FMC2ComponentRow* Row = ComponentTable->FindRow<FMC2ComponentRow>(CompID, TEXT("ValidateLoadout"));
		if (!Row)
			continue;

		Result.TotalWeight += Row->Weight;

		// Distribute slot usage across all allowed locations (component fills its first allowed slot)
		// In MC2, components are placed in a specific location chosen by the player.
		// Here we trust the saved ComponentSlots ordering maps to locations 0-7 in groups:
		// Slots 0 = Head (1 slot), Slots 1-12 = CT, Slots 13-24 = LT, etc.
		// For validation purposes, count slots per location from the component's LocXxx bools.
		for (int32 Loc = 0; Loc < NumLocations; ++Loc)
		{
			if (IsComponentAllowedInLocation(*Row, (EMC2SlotLocation)Loc))
			{
				if (Result.UsedSlots[Loc] + Row->CritHits <= Result.MaxSlots[Loc])
				{
					Result.UsedSlots[Loc] += Row->CritHits;
					break;
				}
			}
		}
	}

	// Check weight
	if (Result.TotalWeight > Result.MaxWeight)
	{
		Result.bIsValid = false;
		Result.ErrorMessage = FString::Printf(
			TEXT("Overweight: %.1f / %.1f tons"),
			Result.TotalWeight, Result.MaxWeight);
		return Result;
	}

	// Check slot overflow per location
	for (int32 i = 0; i < NumLocations; ++i)
	{
		if (Result.UsedSlots[i] > Result.MaxSlots[i])
		{
			Result.bIsValid = false;
			Result.ErrorMessage = FString::Printf(
				TEXT("Too many components in location %d (%d / %d slots)"),
				i, Result.UsedSlots[i], Result.MaxSlots[i]);
			return Result;
		}
	}

	return Result;
}

bool UMC2MechLabValidator::CanAddComponentToLocation(
	const FMC2ComponentRow& ComponentRow,
	EMC2SlotLocation Location,
	const TArray<int32>& CurrentSlots,
	const FMC2MechChassisRow& ChassisRow)
{
	if (!IsComponentAllowedInLocation(ComponentRow, Location))
		return false;

	const int32 LocIdx = (int32)Location;
	const int32 MaxSlots = GetLocationMaxSlots(ChassisRow, Location);
	const int32 Used     = CurrentSlots.IsValidIndex(LocIdx) ? CurrentSlots[LocIdx] : 0;

	return (Used + ComponentRow.CritHits) <= MaxSlots;
}

float UMC2MechLabValidator::CalculateTotalWeight(
	const TArray<FName>& ComponentSlots,
	UDataTable* ComponentTable)
{
	if (!ComponentTable)
		return 0.f;

	float Total = 0.f;
	for (const FName& CompID : ComponentSlots)
	{
		if (CompID.IsNone())
			continue;
		if (FMC2ComponentRow* Row = ComponentTable->FindRow<FMC2ComponentRow>(CompID, TEXT("CalcWeight")))
			Total += Row->Weight;
	}
	return Total;
}

int32 UMC2MechLabValidator::GetLocationMaxSlots(const FMC2MechChassisRow& /*ChassisRow*/, EMC2SlotLocation Location)
{
	// All MC2 Inner Sphere mechs use standard BattleTech slot counts.
	// Head=1, CT=12, LT/RT=12, LA/RA=12, LL/RL=6.
	return DefaultSlots[(int32)Location];
}

bool UMC2MechLabValidator::IsComponentAllowedInLocation(const FMC2ComponentRow& ComponentRow, EMC2SlotLocation Location)
{
	switch (Location)
	{
	case EMC2SlotLocation::Head:         return ComponentRow.LocHead;
	case EMC2SlotLocation::CenterTorso:  return ComponentRow.LocCT;
	case EMC2SlotLocation::LeftTorso:    return ComponentRow.LocLT;
	case EMC2SlotLocation::RightTorso:   return ComponentRow.LocRT;
	case EMC2SlotLocation::LeftArm:      return ComponentRow.LocLA;
	case EMC2SlotLocation::RightArm:     return ComponentRow.LocRA;
	case EMC2SlotLocation::LeftLeg:      return ComponentRow.LocLL;
	case EMC2SlotLocation::RightLeg:     return ComponentRow.LocRL;
	}
	return false;
}

FMC2MechRecord UMC2MechLabValidator::BuildMechRecord(
	FName ChassisID,
	int32 VariantIndex,
	const TArray<FName>& ComponentSlots)
{
	FMC2MechRecord Record;
	Record.ChassisID     = ChassisID;
	Record.VariantIndex  = VariantIndex;
	Record.ComponentSlots = ComponentSlots;
	return Record;
}

// ---------------------------------------------------------------------------
// Tech base
// ---------------------------------------------------------------------------

bool UMC2MechLabValidator::IsComponentAvailable(const FMC2ComponentRow& ComponentRow, bool bClanTechUnlocked)
{
	// IS and mixed-tech components are always available.
	// Clan components require the campaign to have reached the unlock point
	// (UMC2LogisticsSubsystem::IsClanTechUnlocked checks CampaignFlag[32]).
	if (ComponentRow.TechBase == EMC2TechBase::Clan)
		return bClanTechUnlocked;

	return true;
}

TArray<FName> UMC2MechLabValidator::FilterAvailableComponents(UDataTable* ComponentTable, bool bClanTechUnlocked)
{
	TArray<FName> Available;
	if (!ComponentTable)
		return Available;

	for (const FName& RowName : ComponentTable->GetRowNames())
	{
		if (FMC2ComponentRow* Row = ComponentTable->FindRow<FMC2ComponentRow>(RowName, TEXT("FilterAvailable")))
		{
			if (IsComponentAvailable(*Row, bClanTechUnlocked))
				Available.Add(RowName);
		}
	}
	return Available;
}
