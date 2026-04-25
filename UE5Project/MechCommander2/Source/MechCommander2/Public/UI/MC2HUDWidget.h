#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MC2HUDWidget.generated.h"

class AMC2Mover;
class UProgressBar;
class UTextBlock;
class UImage;
class UCanvasPanel;
class UHorizontalBox;
class UMC2MinimapWidget;
class UMC2UnitPortraitWidget;

/**
 * UMC2HUDWidget  (WBP_MC2HUD)
 * Root UMG widget for the tactical HUD.
 * Contains:
 *   - Top bar: mission clock, C-Bills, RP
 *   - Bottom bar: selected unit portraits (up to 6)
 *   - Left panel: heat gauge, armor arc diagram
 *   - Right panel: minimap
 *   - Objective list overlay
 *
 * This class exposes the C++ binding points; layout is done in Blueprint/UMG.
 */
UCLASS(Abstract, BlueprintType)
class MECHCOMMANDER2_API UMC2HUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- Mission clock ---

	UFUNCTION(BlueprintCallable, Category = "HUD|Clock")
	void SetMissionTime(float Seconds);

	// --- Resources ---

	UFUNCTION(BlueprintCallable, Category = "HUD|Resources")
	void SetCBills(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "HUD|Resources")
	void SetResourcePoints(int32 Amount);

	// --- Selection update ---

	// Called by AMC2HUD when the player's selection changes.
	// Widget rebuilds the portrait strip from this list.
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Selection")
	void OnSelectionChanged(const TArray<AMC2Mover*>& SelectedUnits);

	// --- Objective list ---

	// Called by GameMode when objective status changes.
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Objectives")
	void OnObjectivesUpdated();

	// --- Mission result overlay ---

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Result")
	void ShowMissionResult(uint8 Result);   // cast of EMC2MissionResult

	// --- Minimap ---

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UMC2MinimapWidget> Minimap;

	// Bound widgets (set in UMG designer; BindWidget enforces name match)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> MissionClockText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> CBillsText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ResourcePointsText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UHorizontalBox> PortraitStrip;   // populated with UMC2UnitPortraitWidget

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	// Auto-refreshed each tick from focused unit
	void RefreshFocusedUnitPanel(float DeltaTime);

	UPROPERTY()
	TWeakObjectPtr<AMC2Mover> FocusedUnit;
};
