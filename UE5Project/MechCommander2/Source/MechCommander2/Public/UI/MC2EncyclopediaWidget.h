#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "MC2EncyclopediaWidget.generated.h"

/**
 * In-game Encyclopedia / Codex widget.
 * Source: mcl_en.fit (7 category buttons) + mcl_en_mechs.fit / mcl_en_bldg.fit /
 *         mcl_en_wep.fit / mcl_en_person.fit / mcl_en_sub.fit sub-layouts.
 *
 * Layout:
 *   Left column  — 7 category tab buttons (IDs: mechs, vehicles, buildings, weapons,
 *                  pilots, components, misc)
 *   Centre pane  — scrollable entry list for selected category
 *   Right pane   — description scroll box, stat scroll box, weapon list (per entry)
 *   Close button — dismisses widget
 *
 * Data sources: DT_MechChassis, DT_VehicleTypes, DT_BuildingTypes,
 *               DT_Components, DT_Pilots (all populated by ue_import_data_tables.py).
 */

UENUM(BlueprintType)
enum class EMC2EncyclopediaCategory : uint8
{
    Mechs      UMETA(DisplayName = "BattleMechs"),
    Vehicles   UMETA(DisplayName = "Vehicles"),
    Buildings  UMETA(DisplayName = "Buildings"),
    Weapons    UMETA(DisplayName = "Weapons"),
    Pilots     UMETA(DisplayName = "Pilots"),
    Components UMETA(DisplayName = "Components"),
    Misc       UMETA(DisplayName = "Miscellaneous"),
};

USTRUCT(BlueprintType)
struct FMC2EncyclopediaEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FString RowName;
    UPROPERTY(BlueprintReadOnly) FText   DisplayName;
    UPROPERTY(BlueprintReadOnly) FText   Description;
    UPROPERTY(BlueprintReadOnly) FText   Stats;        // preformatted multi-line stat block
    UPROPERTY(BlueprintReadOnly) FText   WeaponList;   // preformatted weapon/component list
    UPROPERTY(BlueprintReadOnly) int32   EncyclopediaID = -1;
};

UCLASS(Abstract)
class MECHCOMMANDER2_API UMC2EncyclopediaWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // ── Data Table refs — assign in BP defaults ───────────────────────────────
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MC2|Encyclopedia|Data")
    TSoftObjectPtr<UDataTable> MechChassisTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MC2|Encyclopedia|Data")
    TSoftObjectPtr<UDataTable> VehicleTypesTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MC2|Encyclopedia|Data")
    TSoftObjectPtr<UDataTable> BuildingTypesTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MC2|Encyclopedia|Data")
    TSoftObjectPtr<UDataTable> ComponentsTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MC2|Encyclopedia|Data")
    TSoftObjectPtr<UDataTable> PilotsTable;

    // ── API ───────────────────────────────────────────────────────────────────

    /** Switch the active category and rebuild the entry list. */
    UFUNCTION(BlueprintCallable, Category = "MC2|Encyclopedia")
    void SelectCategory(EMC2EncyclopediaCategory Category);

    /** Select an entry by row name and populate the detail panes. */
    UFUNCTION(BlueprintCallable, Category = "MC2|Encyclopedia")
    void SelectEntry(const FString& RowName);

    /** Returns all entries for the current category (for list population). */
    UFUNCTION(BlueprintCallable, Category = "MC2|Encyclopedia")
    TArray<FMC2EncyclopediaEntry> GetCurrentEntries() const;

    UFUNCTION(BlueprintCallable, Category = "MC2|Encyclopedia")
    void Close() { RemoveFromParent(); }

    // ── Blueprint events ──────────────────────────────────────────────────────

    UFUNCTION(BlueprintImplementableEvent, Category = "MC2|Encyclopedia")
    void OnCategorySelected(EMC2EncyclopediaCategory NewCategory,
                            const TArray<FMC2EncyclopediaEntry>& Entries);

    UFUNCTION(BlueprintImplementableEvent, Category = "MC2|Encyclopedia")
    void OnEntrySelected(const FMC2EncyclopediaEntry& Entry);

protected:
    UPROPERTY(BlueprintReadOnly, Category = "MC2|Encyclopedia")
    EMC2EncyclopediaCategory CurrentCategory = EMC2EncyclopediaCategory::Mechs;

    UPROPERTY(BlueprintReadOnly, Category = "MC2|Encyclopedia")
    FMC2EncyclopediaEntry CurrentEntry;

    virtual void NativeConstruct() override;

private:
    TArray<FMC2EncyclopediaEntry> BuildEntries(EMC2EncyclopediaCategory Category) const;
    FMC2EncyclopediaEntry         BuildMechEntry(const FName& RowName, const UDataTable* DT) const;
    FMC2EncyclopediaEntry         BuildVehicleEntry(const FName& RowName, const UDataTable* DT) const;
    FMC2EncyclopediaEntry         BuildBuildingEntry(const FName& RowName, const UDataTable* DT) const;
    FMC2EncyclopediaEntry         BuildComponentEntry(const FName& RowName, const UDataTable* DT) const;
    FMC2EncyclopediaEntry         BuildPilotEntry(const FName& RowName, const UDataTable* DT) const;
};
