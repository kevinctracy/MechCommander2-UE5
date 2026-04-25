#include "UI/MC2MainMenuWidget.h"
#include "Campaign/MC2LogisticsSubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UMC2MainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (NewCampaignButton) NewCampaignButton->OnClicked.AddDynamic(this, &UMC2MainMenuWidget::HandleNewCampaign);
	if (LoadGameButton)    LoadGameButton->OnClicked.AddDynamic(this, &UMC2MainMenuWidget::HandleLoadGame);
	if (MultiplayerButton) MultiplayerButton->OnClicked.AddDynamic(this, &UMC2MainMenuWidget::HandleMultiplayer);
	if (OptionsButton)     OptionsButton->OnClicked.AddDynamic(this, &UMC2MainMenuWidget::HandleOptions);
	if (QuitButton)        QuitButton->OnClicked.AddDynamic(this, &UMC2MainMenuWidget::HandleQuit);

	if (VersionText)
		VersionText->SetText(FText::FromString(TEXT("MechCommander 2 UE5 — Build 2026")));

	// Grey out Load if no saves exist
	if (LoadGameButton)
		LoadGameButton->SetIsEnabled(HasSaveInSlot(0) || HasSaveInSlot(1) || HasSaveInSlot(2));
}

bool UMC2MainMenuWidget::HasSaveInSlot(int32 SlotIndex) const
{
	if (UMC2LogisticsSubsystem* LS = GetGameInstance()->GetSubsystem<UMC2LogisticsSubsystem>())
		return LS->HasSaveInSlot(SlotIndex);
	return false;
}

void UMC2MainMenuWidget::LoadSlot(int32 SlotIndex)
{
	if (UMC2LogisticsSubsystem* LS = GetGameInstance()->GetSubsystem<UMC2LogisticsSubsystem>())
	{
		LS->LoadGame(SlotIndex);
		// Transition to MechBay level — map name set in Project Settings
		UGameplayStatics::OpenLevel(this, FName("L_MechBay"));
	}
}

void UMC2MainMenuWidget::StartNewCampaign(const FString& CommanderName)
{
	if (UMC2LogisticsSubsystem* LS = GetGameInstance()->GetSubsystem<UMC2LogisticsSubsystem>())
	{
		LS->NewCampaign(CommanderName, 0);
		UGameplayStatics::OpenLevel(this, FName("L_MechBay"));
	}
}

void UMC2MainMenuWidget::HandleNewCampaign() { OnNewCampaignPressed(); }
void UMC2MainMenuWidget::HandleLoadGame()    { OnLoadGamePressed(); }
void UMC2MainMenuWidget::HandleMultiplayer() { OnMultiplayerPressed(); }
void UMC2MainMenuWidget::HandleOptions()     { OnOptionsPressed(); }
void UMC2MainMenuWidget::HandleQuit()
{
	UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}
