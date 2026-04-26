#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MC2PilotReadyWidget.generated.h"

class UListView;
class UTextBlock;
class UButton;
class UMC2LogisticsSubsystem;

/**
 * UMC2PilotReadyWidget  (WBP_PilotReady)
 * Pre-mission lance assignment screen.
 * Player assigns available pilots to the 4 lance mech slots before launching a mission.
 *
 * Maps to MC2's MCL_PR_*.fit pilot ready screens.
 *
 * Data flow:
 *   1. SetMissionID() loads available mechs + pilots from LogisticsSubsystem
 *   2. Player clicks mech slot → picks a pilot from PilotList
 *   3. OnLaunchReady() is called when all slots are filled
 *   4. LaunchMission() calls LogisticsSubsystem::CommitMissionToSave() then opens the mission level
 */
UCLASS(Abstract, BlueprintType)
class MECHCOMMANDER2_API UMC2PilotReadyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PilotReady")
	void SetMissionID(int32 MissionIndex);

	// Assign a pilot to a lance slot (0-3); clears prior assignment for that pilot
	UFUNCTION(BlueprintCallable, Category = "PilotReady")
	void AssignPilotToSlot(int32 SlotIndex, int32 PilotID);

	// Unassign a slot
	UFUNCTION(BlueprintCallable, Category = "PilotReady")
	void ClearSlot(int32 SlotIndex);

	// Returns true when all 4 lance slots have pilots assigned
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PilotReady")
	bool IsLanceReady() const;

	// Commit assignments and start the mission
	UFUNCTION(BlueprintCallable, Category = "PilotReady")
	void LaunchMission();

	// Blueprint events for refreshing slot display
	UFUNCTION(BlueprintImplementableEvent, Category = "PilotReady")
	void OnSlotAssigned(int32 SlotIndex, int32 PilotID, int32 MechRosterIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "PilotReady")
	void OnSlotCleared(int32 SlotIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "PilotReady")
	void OnLanceReady();

protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UListView> PilotList;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UListView> MechSlotList;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> MissionNameText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>   LaunchButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>   BackButton;

	virtual void NativeConstruct() override;

	UFUNCTION() void HandleLaunch();
	UFUNCTION() void HandleBack();

private:
	int32 CurrentMissionIndex = 0;

	// SlotIndex → PilotID (-1 = empty)
	TArray<int32> LanceSlotPilots = { -1, -1, -1, -1 };
	// SlotIndex → mech roster index
	TArray<int32> LanceSlotMechs  = { -1, -1, -1, -1 };
};
