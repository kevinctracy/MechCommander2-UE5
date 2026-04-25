#include "UI/MC2MissionResultsWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Campaign/MC2LogisticsSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UMC2MissionResultsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ContinueButton) ContinueButton->OnClicked.AddDynamic(this, &UMC2MissionResultsWidget::HandleContinue);
	if (RetryButton)    RetryButton->OnClicked.AddDynamic(this, &UMC2MissionResultsWidget::HandleRetry);
}

void UMC2MissionResultsWidget::SetResults(
	EMC2MissionResult Result, int32 EnemyKills, int32 FriendlyLosses,
	int32 CBillsEarned, int32 XPEarned)
{
	if (ResultBannerText)
		ResultBannerText->SetText(ResultToText(Result));

	if (KillsText)
		KillsText->SetText(FText::Format(FText::FromString(TEXT("Kills: {0}")), FText::AsNumber(EnemyKills)));
	if (LossesText)
		LossesText->SetText(FText::Format(FText::FromString(TEXT("Losses: {0}")), FText::AsNumber(FriendlyLosses)));
	if (CBillsEarnedText)
		CBillsEarnedText->SetText(FText::Format(FText::FromString(TEXT("+{0} C-Bills")), FText::AsNumber(CBillsEarned)));
	if (XPEarnedText)
		XPEarnedText->SetText(FText::Format(FText::FromString(TEXT("+{0} XP")), FText::AsNumber(XPEarned)));

	// Show Retry only for losses
	bool bCanRetry = (Result == EMC2MissionResult::PlayerLostBig || Result == EMC2MissionResult::PlayerLostSmall);
	if (RetryButton) RetryButton->SetVisibility(bCanRetry ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UMC2MissionResultsWidget::AddObjectiveResult(const FText& ObjectiveText, bool bCompleted)
{
	// Caller adds text blocks to ObjectiveResultsList in Blueprint
}

void UMC2MissionResultsWidget::HandleContinue() { OnContinuePressed(); }
void UMC2MissionResultsWidget::HandleRetry()    { OnRetryPressed(); }

FText UMC2MissionResultsWidget::ResultToText(EMC2MissionResult Result)
{
	switch (Result)
	{
	case EMC2MissionResult::PlayerWinBig:   return FText::FromString(TEXT("VICTORY"));
	case EMC2MissionResult::PlayerWinSmall: return FText::FromString(TEXT("PARTIAL VICTORY"));
	case EMC2MissionResult::PlayerLostBig:  return FText::FromString(TEXT("DEFEAT"));
	case EMC2MissionResult::PlayerLostSmall:return FText::FromString(TEXT("PARTIAL DEFEAT"));
	case EMC2MissionResult::Draw:           return FText::FromString(TEXT("DRAW"));
	default:                                return FText::FromString(TEXT("UNKNOWN"));
	}
}
