#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MC2UnitPortraitWidget.generated.h"

class AMC2Mover;
class UProgressBar;
class UTextBlock;
class UImage;
class UButton;

/**
 * UMC2UnitPortraitWidget  (WBP_UnitPortrait)
 * One cell in the bottom portrait strip.
 * Displays:
 *   - Unit icon / silhouette
 *   - Armor bar (green → yellow → red)
 *   - Heat bar (blue → orange)
 *   - Pilot name
 *   - Status icons (shutdown, immobilized, etc.)
 *
 * Clicking the portrait focuses the camera on that unit.
 */
UCLASS(Abstract, BlueprintType)
class MECHCOMMANDER2_API UMC2UnitPortraitWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Bind this portrait to a unit; call once after spawning the widget
	UFUNCTION(BlueprintCallable, Category = "Portrait")
	void BindToUnit(AMC2Mover* Unit);

	// Refresh bars and status from the bound unit (called each tick by HUDWidget)
	UFUNCTION(BlueprintCallable, Category = "Portrait")
	void Refresh();

	// Returns the bound unit (for click handling)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Portrait")
	AMC2Mover* GetBoundUnit() const { return BoundUnit.Get(); }

protected:
	virtual void NativeConstruct() override;

	// UMG BindWidget members — must match designer widget names exactly
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> ArmorBar;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> HeatBar;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock>   PilotNameText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage>       UnitIcon;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>      PortraitButton;

	UFUNCTION()
	void OnPortraitClicked();

private:
	TWeakObjectPtr<AMC2Mover> BoundUnit;

	// Converts an armor percentage to the MC2 traffic-light color scheme
	static FLinearColor ArmorBarColor(float ArmorPct);
};
