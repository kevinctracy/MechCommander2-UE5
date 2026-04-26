#include "Input/MC2GameUserSettings.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "PlayerMappableInputConfig.h"

UMC2GameUserSettings* UMC2GameUserSettings::GetMC2GameUserSettings()
{
	return Cast<UMC2GameUserSettings>(UGameUserSettings::GetGameUserSettings());
}

// ---------- keybinding helpers ----------

int32 UMC2GameUserSettings::FindBindingIndex(const UInputAction* Action) const
{
	if (!Action) { return INDEX_NONE; }
	const FString Path = Action->GetPathName();
	return KeyBindings.IndexOfByPredicate([&Path](const FMC2KeyBinding& B)
	{
		return B.ActionPath == Path;
	});
}

void UMC2GameUserSettings::RemapKey(const UInputAction* Action, FKey NewKey)
{
	if (!Action) { return; }
	const FString Path = Action->GetPathName();
	const int32 Idx    = FindBindingIndex(Action);
	if (Idx == INDEX_NONE)
	{
		FMC2KeyBinding& New = KeyBindings.AddDefaulted_GetRef();
		New.ActionPath       = Path;
		New.MappedKey        = NewKey;
	}
	else
	{
		KeyBindings[Idx].MappedKey = NewKey;
	}
}

void UMC2GameUserSettings::ResetKeyToDefault(const UInputAction* Action)
{
	const int32 Idx = FindBindingIndex(Action);
	if (Idx != INDEX_NONE)
	{
		KeyBindings.RemoveAt(Idx);
	}
}

void UMC2GameUserSettings::ResetAllKeysToDefault()
{
	KeyBindings.Empty();
}

FKey UMC2GameUserSettings::GetMappedKey(const UInputAction* Action) const
{
	const int32 Idx = FindBindingIndex(Action);
	return (Idx != INDEX_NONE) ? KeyBindings[Idx].MappedKey : EKeys::Invalid;
}

// ---------- Apply to Enhanced Input ----------

void UMC2GameUserSettings::ApplyKeyBindings(UEnhancedInputLocalPlayerSubsystem* Subsystem)
{
	if (!Subsystem) { return; }

	UEnhancedInputUserSettings* UserSettings = Subsystem->GetUserSettings();
	if (!UserSettings) { return; }

	for (const FMC2KeyBinding& Binding : KeyBindings)
	{
		const UInputAction* Action = Cast<UInputAction>(
			FSoftObjectPath(Binding.ActionPath).TryLoad());
		if (!Action || !Binding.MappedKey.IsValid()) { continue; }

		// Build a remap request: replace the first slot of the default mapping
		FMapPlayerKeyArgs Args;
		Args.MappingName    = FName(*Action->GetName()); // IMC must use same name
		Args.Slot           = EPlayerMappableKeySlot::First;
		Args.NewKey         = Binding.MappedKey;

		FGameplayTagContainer FailReason;
		UserSettings->MapPlayerKey(Args, FailReason);
	}

	UserSettings->ApplySettings();
}

// ---------- UGameUserSettings overrides ----------

void UMC2GameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	Super::ApplySettings(bCheckForCommandLineOverrides);
	SaveConfig();
}

void UMC2GameUserSettings::LoadSettings(bool bForceReload)
{
	Super::LoadSettings(bForceReload);
	// KeyBindings and ColorblindMode populated automatically from .ini via UPROPERTY(Config)
}

// ---------- Colorblind mode ----------

void UMC2GameUserSettings::SetColorblindMode(EMC2ColorblindMode Mode)
{
	if (ColorblindMode != Mode)
	{
		ColorblindMode = Mode;
		SaveConfig();
		OnColorblindSettingsChanged.Broadcast();
	}
}
