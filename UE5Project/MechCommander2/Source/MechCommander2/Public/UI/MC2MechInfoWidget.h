#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "MC2MechInfoWidget.generated.h"

/**
 * Mech purchasing / info popup widget.
 * Source: mcl_mechinfo.fit
 *
 * Layout (from FIT):
 *   - Component info scroll area  (491,345 — 227×165)
 *   - Mech 3D preview viewport   (338,346 — 124×162), flanked by two vertical separator lines
 *   - Armor diagram               (separate panel showing zone-by-zone armor values)
 *   - Weapon/component list       (scrollable)
 *
 * Shown from WBP_MechBay when the user hovers or selects a mech in the roster or
 * purchase list. Also shown from WBP_Encyclopedia for the mech detail pane.
 */

UCLASS(Abstract)
class MECHCOMMANDER2_API UMC2MechInfoWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MC2|MechInfo|Data")
    TSoftObjectPtr<UDataTable> MechChassisTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MC2|MechInfo|Data")
    TSoftObjectPtr<UDataTable> ComponentsTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MC2|MechInfo|Data")
    TSoftObjectPtr<UDataTable> MechVariantsTable;

    // ── API ───────────────────────────────────────────────────────────────────

    /**
     * Populate the widget from DT_MechChassis row name.
     * VariantIndex selects a variant loadout row from DT_MechVariants.
     * Pass -1 to show base chassis stats without a variant.
     */
    UFUNCTION(BlueprintCallable, Category = "MC2|MechInfo")
    void SetMech(const FName& ChassisRowName, int32 VariantIndex = -1);

    /** Repair cost in C-Bills for the current mech (0 if not in roster context). */
    UFUNCTION(BlueprintCallable, Category = "MC2|MechInfo")
    void SetRepairCost(int32 CostCBills);

    /** Purchase cost override (from campaign pricing table). */
    UFUNCTION(BlueprintCallable, Category = "MC2|MechInfo")
    void SetPurchaseCost(int32 CostCBills);

    UFUNCTION(BlueprintCallable, Category = "MC2|MechInfo")
    void Close() { RemoveFromParent(); }

    // ── Blueprint events ──────────────────────────────────────────────────────

    /**
     * Called after SetMech — provides preformatted strings for each panel.
     * ArmorValues: 11 floats [Head, LA, RA, LT, RT, CT, LL, RL, RearLT, RearRT, RearCT]
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "MC2|MechInfo")
    void OnMechDataSet(const FText& MechName, float Tonnage,
                       float MaxSpeed, int32 BattleRating, int32 ChassisBR,
                       const FText& StatsText,
                       const TArray<int32>& MaxArmorValues,
                       const FText& ComponentListText);

    UFUNCTION(BlueprintImplementableEvent, Category = "MC2|MechInfo")
    void OnCostsSet(int32 RepairCost, int32 PurchaseCost);

protected:
    virtual void NativeConstruct() override;

private:
    FName CurrentChassis;
    int32 CurrentVariant = -1;

    FText BuildComponentList(const FName& ChassisRowName, int32 VariantIndex) const;
    TArray<int32> GetMaxArmorValues(const FName& ChassisRowName) const;
};
