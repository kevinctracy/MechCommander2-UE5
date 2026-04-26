#include "UI/MC2MechInfoWidget.h"
#include "MC2DataTableRows.h"

void UMC2MechInfoWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UMC2MechInfoWidget::SetMech(const FName& ChassisRowName, int32 VariantIndex)
{
    CurrentChassis = ChassisRowName;
    CurrentVariant = VariantIndex;

    UDataTable* ChassisTable = MechChassisTable.Get();
    if (!ChassisTable) return;

    const FMC2MechChassisRow* Row = ChassisTable->FindRow<FMC2MechChassisRow>(
        ChassisRowName, TEXT("UMC2MechInfoWidget::SetMech"));
    if (!Row) return;

    FString Stats = FString::Printf(
        TEXT("Tonnage:      %d t\n"
             "Top Speed:    %.0f km/h\n"
             "Torso Yaw:    ±%d°\n"
             "Max Armor:    %d pts\n"
             "Battle Rating: %d\n"
             "Repair Cost:  %d CB/pt"),
        (int32)Row->Tonnage,
        Row->MaxRunSpeed,
        Row->MaxTorsoYaw,
        Row->MaxArmor,
        Row->ChassisBR,
        500);   // 500 C-Bills per armor point — matches LogisticsSubsystem::RepairMech

    FText ComponentList = BuildComponentList(ChassisRowName, VariantIndex);
    TArray<int32> ArmorValues = GetMaxArmorValues(ChassisRowName);

    OnMechDataSet(
        FText::FromString(Row->MechName),
        Row->Tonnage,
        Row->MaxRunSpeed,
        Row->ChassisBR,
        Row->ChassisBR,
        FText::FromString(Stats),
        ArmorValues,
        ComponentList);
}

void UMC2MechInfoWidget::SetRepairCost(int32 CostCBills)
{
    OnCostsSet(CostCBills, 0);
}

void UMC2MechInfoWidget::SetPurchaseCost(int32 CostCBills)
{
    OnCostsSet(0, CostCBills);
}

FText UMC2MechInfoWidget::BuildComponentList(const FName& ChassisRowName,
                                              int32 VariantIndex) const
{
    UDataTable* VarTable  = MechVariantsTable.Get();
    UDataTable* CompTable = ComponentsTable.Get();
    if (!VarTable || !CompTable || VariantIndex < 0)
        return FText::GetEmpty();

    FName VarRow = FName(*(ChassisRowName.ToString() + FString::Printf(TEXT("_%d"), VariantIndex)));
    const FMC2MechVariantRow* Variant = VarTable->FindRow<FMC2MechVariantRow>(VarRow, TEXT(""));
    if (!Variant) return FText::GetEmpty();

    FString List;
    auto AppendItem = [&](int32 ItemID)
    {
        if (ItemID <= 0) return;
        const FMC2ComponentRow* Comp = CompTable->FindRow<FMC2ComponentRow>(
            FName(*FString::FromInt(ItemID)), TEXT(""));
        if (Comp)
            List += FString::Printf(TEXT("• %s (%.1ft, %.1f dmg)\n"),
                *Comp->Name, Comp->Weight, Comp->Damage);
    };

    AppendItem(Variant->Item0);  AppendItem(Variant->Item1);
    AppendItem(Variant->Item2);  AppendItem(Variant->Item3);
    AppendItem(Variant->Item4);  AppendItem(Variant->Item5);
    AppendItem(Variant->Item6);  AppendItem(Variant->Item7);
    AppendItem(Variant->Item8);  AppendItem(Variant->Item9);
    AppendItem(Variant->Item10); AppendItem(Variant->Item11);
    AppendItem(Variant->Item12); AppendItem(Variant->Item13);
    AppendItem(Variant->Item14); AppendItem(Variant->Item15);
    AppendItem(Variant->Item16); AppendItem(Variant->Item17);
    AppendItem(Variant->Item18); AppendItem(Variant->Item19);

    return FText::FromString(List);
}

TArray<int32> UMC2MechInfoWidget::GetMaxArmorValues(const FName& ChassisRowName) const
{
    TArray<int32> Values;
    UDataTable* DT = MechChassisTable.Get();
    if (!DT) return Values;

    const FMC2MechChassisRow* Row = DT->FindRow<FMC2MechChassisRow>(ChassisRowName, TEXT(""));
    if (!Row) return Values;

    // 11 zones in the order used by UMC2HealthComponent and CurrentArmor[] in save data:
    // Head, LA, RA, LT, RT, CT, LL, RL, RearLT, RearRT, RearCT
    Values = {
        Row->Armor_Head,
        Row->Armor_LA,   Row->Armor_RA,
        Row->Armor_LT,   Row->Armor_RT,   Row->Armor_CT,
        Row->Armor_LL,   Row->Armor_RL,
        Row->Armor_RearLT, Row->Armor_RearRT, Row->Armor_RearCT
    };
    return Values;
}
