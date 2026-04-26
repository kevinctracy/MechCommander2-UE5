#include "UI/MC2EncyclopediaWidget.h"
#include "MC2DataTableRows.h"

void UMC2EncyclopediaWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SelectCategory(EMC2EncyclopediaCategory::Mechs);
}

void UMC2EncyclopediaWidget::SelectCategory(EMC2EncyclopediaCategory Category)
{
    CurrentCategory = Category;
    TArray<FMC2EncyclopediaEntry> Entries = BuildEntries(Category);
    OnCategorySelected(Category, Entries);
}

void UMC2EncyclopediaWidget::SelectEntry(const FString& RowName)
{
    TArray<FMC2EncyclopediaEntry> Entries = BuildEntries(CurrentCategory);
    for (const FMC2EncyclopediaEntry& E : Entries)
    {
        if (E.RowName == RowName)
        {
            CurrentEntry = E;
            OnEntrySelected(E);
            return;
        }
    }
}

TArray<FMC2EncyclopediaEntry> UMC2EncyclopediaWidget::GetCurrentEntries() const
{
    return BuildEntries(CurrentCategory);
}

TArray<FMC2EncyclopediaEntry> UMC2EncyclopediaWidget::BuildEntries(
    EMC2EncyclopediaCategory Category) const
{
    TArray<FMC2EncyclopediaEntry> Result;
    switch (Category)
    {
    case EMC2EncyclopediaCategory::Mechs:
        if (UDataTable* DT = MechChassisTable.Get())
            for (const FName& Row : DT->GetRowNames())
                Result.Add(BuildMechEntry(Row, DT));
        break;
    case EMC2EncyclopediaCategory::Vehicles:
        if (UDataTable* DT = VehicleTypesTable.Get())
            for (const FName& Row : DT->GetRowNames())
                Result.Add(BuildVehicleEntry(Row, DT));
        break;
    case EMC2EncyclopediaCategory::Buildings:
        if (UDataTable* DT = BuildingTypesTable.Get())
            for (const FName& Row : DT->GetRowNames())
                Result.Add(BuildBuildingEntry(Row, DT));
        break;
    case EMC2EncyclopediaCategory::Weapons:
    case EMC2EncyclopediaCategory::Components:
        if (UDataTable* DT = ComponentsTable.Get())
            for (const FName& Row : DT->GetRowNames())
                Result.Add(BuildComponentEntry(Row, DT));
        break;
    case EMC2EncyclopediaCategory::Pilots:
        if (UDataTable* DT = PilotsTable.Get())
            for (const FName& Row : DT->GetRowNames())
                Result.Add(BuildPilotEntry(Row, DT));
        break;
    default:
        break;
    }
    return Result;
}

FMC2EncyclopediaEntry UMC2EncyclopediaWidget::BuildMechEntry(const FName& RowName,
                                                              const UDataTable* DT) const
{
    const FMC2MechChassisRow* Row = DT->FindRow<FMC2MechChassisRow>(RowName, TEXT(""));
    if (!Row) return {};

    FMC2EncyclopediaEntry E;
    E.RowName      = RowName.ToString();
    E.DisplayName  = FText::FromString(Row->MechName);
    E.EncyclopediaID = Row->EncyclopediaID;

    E.Stats = FText::FromString(FString::Printf(
        TEXT("Tonnage:    %d t\nTop Speed:  %.0f km/h\nMax Armor:  %d pts\nBR:         %d"),
        (int32)Row->Tonnage, Row->MaxRunSpeed, Row->MaxArmor, Row->ChassisBR));

    FString IS;
    IS += FString::Printf(TEXT("Head %d | CT %d | LT %d | RT %d\n"),
        Row->IS_Head, Row->IS_CenterTorso, Row->IS_LeftTorso, Row->IS_RightTorso);
    IS += FString::Printf(TEXT("LA %d | RA %d | LL %d | RL %d"),
        Row->IS_LeftArm, Row->IS_RightArm, Row->IS_LeftLeg, Row->IS_RightLeg);
    E.WeaponList = FText::FromString(IS);

    return E;
}

FMC2EncyclopediaEntry UMC2EncyclopediaWidget::BuildVehicleEntry(const FName& RowName,
                                                                 const UDataTable* DT) const
{
    const FMC2VehicleTypeRow* Row = DT->FindRow<FMC2VehicleTypeRow>(RowName, TEXT(""));
    if (!Row) return {};

    FMC2EncyclopediaEntry E;
    E.RowName    = RowName.ToString();
    E.DisplayName = FText::FromString(Row->Name);

    E.Stats = FText::FromString(FString::Printf(
        TEXT("Tonnage:    %.0f t\nMax Speed:  %.0f km/h\nBR:         %d"),
        Row->Tonnage, Row->MaxVelocity, Row->BattleRating));

    FString Armor;
    Armor += FString::Printf(TEXT("F:%d R:%d L:%d R:%d T:%d"),
        Row->Armor_Front, Row->Armor_Rear, Row->Armor_Left,
        Row->Armor_Right, Row->Armor_Turret);
    E.WeaponList = FText::FromString(Armor);
    return E;
}

FMC2EncyclopediaEntry UMC2EncyclopediaWidget::BuildBuildingEntry(const FName& RowName,
                                                                  const UDataTable* DT) const
{
    const FMC2BuildingTypeRow* Row = DT->FindRow<FMC2BuildingTypeRow>(RowName, TEXT(""));
    if (!Row) return {};

    FMC2EncyclopediaEntry E;
    E.RowName    = RowName.ToString();
    E.DisplayName = FText::FromString(Row->Name);
    E.Stats       = FText::FromString(FString::Printf(TEXT("Radius: %.0f cm"), Row->ExtentRadius));
    return E;
}

FMC2EncyclopediaEntry UMC2EncyclopediaWidget::BuildComponentEntry(const FName& RowName,
                                                                   const UDataTable* DT) const
{
    const FMC2ComponentRow* Row = DT->FindRow<FMC2ComponentRow>(RowName, TEXT(""));
    if (!Row) return {};

    FMC2EncyclopediaEntry E;
    E.RowName    = RowName.ToString();
    E.DisplayName = FText::FromString(Row->Name);
    E.EncyclopediaID = Row->EncyclopediaIndex;

    E.Stats = FText::FromString(FString::Printf(
        TEXT("Type:     %s\nWeight:   %.1f t\nHeat:     %.1f\nDamage:   %.1f\nRange:    %.0f m\nBR:       %d"),
        *Row->Type, Row->Weight, Row->Heat, Row->Damage, Row->Range, Row->BattleRating));
    return E;
}

FMC2EncyclopediaEntry UMC2EncyclopediaWidget::BuildPilotEntry(const FName& RowName,
                                                               const UDataTable* DT) const
{
    const FMC2PilotRow* Row = DT->FindRow<FMC2PilotRow>(RowName, TEXT(""));
    if (!Row) return {};

    FMC2EncyclopediaEntry E;
    E.RowName    = RowName.ToString();
    E.DisplayName = FText::FromString(FString::Printf(TEXT("%s %s"), *Row->Rank, *Row->PilotName));

    E.Stats = FText::FromString(FString::Printf(
        TEXT("Gunnery:  %d\nPiloting: %d"),
        Row->Gunnery, Row->Piloting));
    return E;
}
