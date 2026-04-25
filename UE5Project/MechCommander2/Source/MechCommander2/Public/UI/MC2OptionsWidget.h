#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MC2OptionsWidget.generated.h"

class USlider;
class UCheckBox;
class UComboBoxString;
class UButton;
class UTextBlock;

/**
 * UMC2OptionsWidget  (WBP_Options)
 * Settings screen — Graphics, Audio, and keybindings.
 * Reads/writes UGameUserSettings for persistence.
 * Keybindings use UEnhancedInputUserSettings for remapping.
 */
UCLASS(Abstract, BlueprintType)
class MECHCOMMANDER2_API UMC2OptionsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Load current settings into all widgets
	UFUNCTION(BlueprintCallable, Category = "Options")
	void LoadCurrentSettings();

	// Apply and save all settings
	UFUNCTION(BlueprintCallable, Category = "Options")
	void ApplyAndSave();

	// Revert to saved settings without applying
	UFUNCTION(BlueprintCallable, Category = "Options")
	void RevertChanges();

	UFUNCTION(BlueprintImplementableEvent, Category = "Options")
	void OnClosed();

protected:
	// --- Audio ---
	UPROPERTY(meta=(BindWidget)) TObjectPtr<USlider>       MasterVolumeSlider;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<USlider>       MusicVolumeSlider;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<USlider>       SFXVolumeSlider;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<USlider>       VOVolumeSlider;

	// --- Graphics ---
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UComboBoxString> ResolutionBox;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UComboBoxString> QualityPresetBox;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UCheckBox>       VSyncCheck;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UCheckBox>       FullscreenCheck;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<USlider>         GammaSlider;

	// --- Gameplay ---
	UPROPERTY(meta=(BindWidget)) TObjectPtr<USlider>       ScrollSpeedSlider;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UCheckBox>     EdgeScrollCheck;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UCheckBox>     ShowDamageNumbersCheck;

	// --- Controls ---
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>       ApplyButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>       RevertButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>       CloseButton;

	virtual void NativeConstruct() override;

	UFUNCTION() void HandleApply();
	UFUNCTION() void HandleRevert();
	UFUNCTION() void HandleClose();

private:
	void ApplyAudioSettings();
	void ApplyGraphicsSettings();
};
