#include "UI/MC2OptionsWidget.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"

void UMC2OptionsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ApplyButton)  ApplyButton->OnClicked.AddDynamic(this, &UMC2OptionsWidget::HandleApply);
	if (RevertButton) RevertButton->OnClicked.AddDynamic(this, &UMC2OptionsWidget::HandleRevert);
	if (CloseButton)  CloseButton->OnClicked.AddDynamic(this, &UMC2OptionsWidget::HandleClose);

	LoadCurrentSettings();
}

void UMC2OptionsWidget::LoadCurrentSettings()
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings) return;

	// Audio — read from sound mix / game settings (stored as custom config vars)
	// These would be stored in UMC2GameUserSettings if we subclass UGameUserSettings
	if (MasterVolumeSlider) MasterVolumeSlider->SetValue(1.f);
	if (MusicVolumeSlider)  MusicVolumeSlider->SetValue(0.7f);
	if (SFXVolumeSlider)    SFXVolumeSlider->SetValue(1.f);
	if (VOVolumeSlider)     VOVolumeSlider->SetValue(1.f);

	// Graphics
	FIntPoint Res = Settings->GetScreenResolution();
	if (ResolutionBox)
		ResolutionBox->SetSelectedOption(FString::Printf(TEXT("%dx%d"), Res.X, Res.Y));

	if (VSyncCheck)    VSyncCheck->SetIsChecked(Settings->IsVSyncEnabled());
	if (FullscreenCheck)
		FullscreenCheck->SetIsChecked(Settings->GetFullscreenMode() == EWindowMode::Fullscreen);
	if (GammaSlider)   GammaSlider->SetValue(Settings->GetGamma());

	// Gameplay
	if (EdgeScrollCheck)    EdgeScrollCheck->SetIsChecked(true);
	if (ScrollSpeedSlider)  ScrollSpeedSlider->SetValue(0.5f);
}

void UMC2OptionsWidget::ApplyAndSave()
{
	ApplyAudioSettings();
	ApplyGraphicsSettings();

	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings)
	{
		Settings->ApplySettings(false);
		Settings->SaveSettings();
	}
}

void UMC2OptionsWidget::RevertChanges()
{
	LoadCurrentSettings();
}

void UMC2OptionsWidget::ApplyAudioSettings()
{
	// Set sound mix class volumes — requires SoundMix asset reference passed in from BP
	// UGameplayStatics::SetSoundMixClassOverride(this, SoundMix, MusicClass, MusicVolumeSlider->GetValue(), ...)
}

void UMC2OptionsWidget::ApplyGraphicsSettings()
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings) return;

	if (VSyncCheck)
		Settings->SetVSyncEnabled(VSyncCheck->IsChecked());
	if (FullscreenCheck)
		Settings->SetFullscreenMode(FullscreenCheck->IsChecked() ? EWindowMode::Fullscreen : EWindowMode::Windowed);
	if (GammaSlider)
		Settings->SetGamma(GammaSlider->GetValue());
}

void UMC2OptionsWidget::HandleApply()  { ApplyAndSave(); }
void UMC2OptionsWidget::HandleRevert() { RevertChanges(); }
void UMC2OptionsWidget::HandleClose()  { OnClosed(); }
