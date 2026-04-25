#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Campaign/MC2SaveGame.h"
#include "MC2MechBayWidget.generated.h"

class UListView;
class UTextBlock;
class UButton;
class UImage;
class UProgressBar;

/**
 * UMC2MechBayWidget  (WBP_MechBay)
 * Between-mission logistics screen for mech roster management.
 * Matches MC2's MechBay / MechLab front page:
 *   Left panel:  mech roster list (UListView of FMC2MechRecord)
 *   Right panel: selected mech stats, repair cost, pilot slot
 *   Bottom bar:  C-Bill balance, RP, "Launch Mission" button
 *
 * All selection state and purchase/repair logic implemented in Blueprint
 * using the UMC2LogisticsSubsystem API.
 */
UCLASS(Abstract, BlueprintType)
class MECHCOMMANDER2_API UMC2MechBayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Refresh the mech list from current save data
	UFUNCTION(BlueprintCallable, Category = "MechBay")
	void RefreshRoster();

	// Show stats for a specific mech roster index
	UFUNCTION(BlueprintCallable, Category = "MechBay")
	void SelectMech(int32 RosterIndex);

	// Spend C-Bills to repair selected mech to full
	UFUNCTION(BlueprintCallable, Category = "MechBay")
	bool RepairSelectedMech();

	// Assign a pilot from the roster to the selected mech
	UFUNCTION(BlueprintCallable, Category = "MechBay")
	bool AssignPilotToSelected(FName PilotID);

	// Events implemented in Blueprint
	UFUNCTION(BlueprintImplementableEvent, Category = "MechBay")
	void OnMechSelected(int32 RosterIndex, const FMC2MechRecord& MechData);

	UFUNCTION(BlueprintImplementableEvent, Category = "MechBay")
	void OnRosterRefreshed();

	UFUNCTION(BlueprintImplementableEvent, Category = "MechBay")
	void OnRepairCompleted(int32 RepairCost);

protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UListView>  MechRosterList;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> CBillsText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> RepairCostText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> MechNameText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> PilotNameText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>    RepairButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>    LaunchMissionButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> ArmorBar;

	virtual void NativeConstruct() override;

	UFUNCTION() void HandleRepairClicked();
	UFUNCTION() void HandleLaunchMission();

private:
	int32 SelectedRosterIndex = -1;

	// C-Bill cost per point of armor damage (from original game balance)
	static constexpr int32 CBILLS_PER_ARMOR_POINT = 500;
};
