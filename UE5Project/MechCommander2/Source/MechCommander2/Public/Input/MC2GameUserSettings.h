#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "InputCoreTypes.h"
#include "MC2GameUserSettings.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnColorblindSettingsChanged);

UENUM(BlueprintType)
enum class EMC2ColorblindMode : uint8
{
	None          UMETA(DisplayName = "Off"),
	Deuteranopia  UMETA(DisplayName = "Deuteranopia (Red-Green)"),
	Protanopia    UMETA(DisplayName = "Protanopia (Red-Green, severe)"),
	Tritanopia    UMETA(DisplayName = "Tritanopia (Blue-Yellow)"),
};

class UInputAction;
class UEnhancedInputLocalPlayerSubsystem;

/**
 * FMC2KeyBinding
 * Stores a player-remapped key for a single Input Action.
 * Serialised into GameUserSettings.ini via SaveConfig().
 */
USTRUCT(BlueprintType)
struct FMC2KeyBinding
{
	GENERATED_BODY()

	// Asset path of the UInputAction (e.g. "/Game/Input/Actions/IA_CameraPan")
	UPROPERTY(Config, BlueprintReadOnly)
	FString ActionPath;

	// The key the player chose
	UPROPERTY(Config, BlueprintReadOnly)
	FKey MappedKey = EKeys::Invalid;
};

/**
 * UMC2GameUserSettings
 * Extends UGameUserSettings with MC2-specific keybinding persistence.
 *
 * Usage:
 *   UMC2GameUserSettings* S = UMC2GameUserSettings::GetMC2GameUserSettings();
 *   S->RemapKey(IA_CameraPan, EKeys::W);
 *   S->ApplyKeyBindings(EnhancedInputSubsystem);
 *   S->SaveSettings();
 *
 * Set GameUserSettingsClassName=MechCommander2.MC2GameUserSettings in DefaultEngine.ini.
 */
UCLASS(Config=GameUserSettings, configdonotcheckdefaults)
class MECHCOMMANDER2_API UMC2GameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	// Convenience accessor (safe cast of GetGameUserSettings())
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Settings")
	static UMC2GameUserSettings* GetMC2GameUserSettings();

	// --- Keybinding API ---

	// Record a player remap (does not apply to subsystem yet)
	UFUNCTION(BlueprintCallable, Category = "Settings|Input")
	void RemapKey(const UInputAction* Action, FKey NewKey);

	// Remove a player override so the default mapping is used
	UFUNCTION(BlueprintCallable, Category = "Settings|Input")
	void ResetKeyToDefault(const UInputAction* Action);

	// Clear all overrides
	UFUNCTION(BlueprintCallable, Category = "Settings|Input")
	void ResetAllKeysToDefault();

	// Return the currently saved key for an action (EKeys::Invalid if not remapped)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Settings|Input")
	FKey GetMappedKey(const UInputAction* Action) const;

	// Push all stored overrides into the Enhanced Input subsystem mapping context
	UFUNCTION(BlueprintCallable, Category = "Settings|Input")
	void ApplyKeyBindings(UEnhancedInputLocalPlayerSubsystem* Subsystem);

	// --- Accessibility: Colorblind Mode ---

	// Fired whenever colorblind settings change; WBP binds to this
	UPROPERTY(BlueprintAssignable, Category = "Settings|Accessibility")
	FOnColorblindSettingsChanged OnColorblindSettingsChanged;

	UFUNCTION(BlueprintCallable, Category = "Settings|Accessibility")
	void SetColorblindMode(EMC2ColorblindMode Mode);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Settings|Accessibility")
	EMC2ColorblindMode GetColorblindMode() const { return ColorblindMode; }

	// Whether UI indicators use shape differentiation instead of only colour
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Settings|Accessibility")
	bool IsColorblindModeActive() const { return ColorblindMode != EMC2ColorblindMode::None; }

	// Subtitle display
	UPROPERTY(Config, BlueprintReadWrite, Category = "Settings|Accessibility")
	bool bSubtitlesEnabled = true;

	// UGameUserSettings interface
	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;
	virtual void LoadSettings(bool bForceReload = false) override;

private:
	UPROPERTY(Config)
	TArray<FMC2KeyBinding> KeyBindings;

	UPROPERTY(Config)
	EMC2ColorblindMode ColorblindMode = EMC2ColorblindMode::None;

	// Returns the index in KeyBindings for the given action, or INDEX_NONE
	int32 FindBindingIndex(const UInputAction* Action) const;
};
