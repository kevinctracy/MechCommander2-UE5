#include "UI/MC2LoadingWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"

void UMC2LoadingWidget::SetMissionInfo(const FText& MissionName, const FText& BriefingExcerpt, UTexture2D* LoadImage)
{
	if (MissionNameText) MissionNameText->SetText(MissionName);
	if (BriefingText)    BriefingText->SetText(BriefingExcerpt);
	if (LoadingImage && LoadImage)
	{
		LoadingImage->SetBrushFromTexture(LoadImage, false);
	}
}

void UMC2LoadingWidget::SetProgress(float Progress)
{
	const float Clamped = FMath::Clamp(Progress, 0.f, 1.f);
	if (LoadingBar) LoadingBar->SetPercent(Clamped);
}

void UMC2LoadingWidget::SetTip(const FText& Tip)
{
	if (TipText) TipText->SetText(Tip);
}
