#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MC2HUD.generated.h"

class UMC2HUDWidget;
class UMC2SelectionBoxWidget;
class AMC2Mover;

/**
 * AMC2HUD
 * Manages the UMG widget stack and renders the rubber-band selection box.
 * All gameplay UI lives in UMG (WBP_MC2HUD); this class is the C++ liaison.
 *
 * Maps to MC2's tacMapInterface + selectionManager draw calls.
 */
UCLASS()
class MECHCOMMANDER2_API AMC2HUD : public AHUD
{
	GENERATED_BODY()

public:
	AMC2HUD();

	// Widget class to instantiate on BeginPlay (set in Blueprint defaults)
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMC2HUDWidget> HUDWidgetClass;

	// The live widget instance
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UMC2HUDWidget> HUDWidget;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "UI")
	UMC2HUDWidget* GetHUDWidget() const { return HUDWidget; }

	// --- Selection box (drawn in raw HUD pass) ---

	// Call from PlayerController when drag begins
	UFUNCTION(BlueprintCallable, Category = "UI|Selection")
	void BeginSelectionBox(const FVector2D& AnchorScreen);

	// Call from PlayerController each tick while dragging
	UFUNCTION(BlueprintCallable, Category = "UI|Selection")
	void UpdateSelectionBox(const FVector2D& CurrentScreen);

	// Call from PlayerController when drag ends
	UFUNCTION(BlueprintCallable, Category = "UI|Selection")
	void EndSelectionBox();

	// Returns the current selection rectangle in screen space
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "UI|Selection")
	FBox2D GetSelectionRect() const;

	// --- Unit portrait / info panel ---

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetSelectedUnits(const TArray<AMC2Mover*>& Units);

	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

private:
	bool      bDrawingSelection = false;
	FVector2D SelectionAnchor   = FVector2D::ZeroVector;
	FVector2D SelectionCurrent  = FVector2D::ZeroVector;

	static constexpr float BOX_THICKNESS = 1.5f;
	static const    FLinearColor BOX_COLOR;
};
