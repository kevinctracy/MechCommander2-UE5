#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Campaign/MC2SaveGame.h"
#include "MC2MechLabWidget.generated.h"

class UListView;
class UTextBlock;
class USlider;
class UButton;
class UUniformGridPanel;

/**
 * UMC2MechLabWidget  (WBP_MechLab)
 * Component loadout editor.
 * Displays a grid of component slots per body section.
 * Validates weight and slot constraints in C++.
 * Drag-and-drop handled in Blueprint via UMG DragDrop.
 *
 * Maps to MC2's MechLab / LogisticsVariant editing flow.
 */
UCLASS(Abstract, BlueprintType)
class MECHCOMMANDER2_API UMC2MechLabWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Load a mech record into the editor; populates all slot widgets
	UFUNCTION(BlueprintCallable, Category = "MechLab")
	void LoadMech(int32 RosterIndex);

	// Attempt to place a component into a slot; returns false if validation fails
	UFUNCTION(BlueprintCallable, Category = "MechLab")
	bool PlaceComponent(FName ComponentID, int32 SlotIndex);

	// Remove a component from a slot
	UFUNCTION(BlueprintCallable, Category = "MechLab")
	void ClearSlot(int32 SlotIndex);

	// Save current loadout back to the logistics subsystem
	UFUNCTION(BlueprintCallable, Category = "MechLab")
	void SaveLoadout();

	// --- Validation queries ---

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MechLab")
	float GetCurrentTonnage() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MechLab")
	float GetMaxTonnage() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MechLab")
	bool IsOverweight() const { return GetCurrentTonnage() > GetMaxTonnage(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MechLab")
	int32 GetUsedSlots(int32 LocationIndex) const;

	// Events
	UFUNCTION(BlueprintImplementableEvent, Category = "MechLab")
	void OnLoadoutChanged();

	UFUNCTION(BlueprintImplementableEvent, Category = "MechLab")
	void OnValidationFailed(const FText& Reason);

protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock>        TonnageText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock>        MechNameText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock>        CBillValueText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UUniformGridPanel> SlotGrid;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>           SaveButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>           ResetButton;

	virtual void NativeConstruct() override;

private:
	int32 EditingRosterIndex = -1;
	FMC2MechRecord WorkingCopy;  // edits staged here until SaveLoadout()
};
