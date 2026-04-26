#include "UI/MC2MissionBriefingWidget.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"

void UMC2MissionBriefingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (LaunchButton) LaunchButton->OnClicked.AddDynamic(this, &UMC2MissionBriefingWidget::HandleLaunch);
	if (BackButton)   BackButton->OnClicked.AddDynamic(this, &UMC2MissionBriefingWidget::HandleBack);
}

void UMC2MissionBriefingWidget::SetMissionData(const FText& Title, const FText& Briefing, UTexture2D* MapOverview)
{
	if (MissionTitleText)  MissionTitleText->SetText(Title);
	if (BriefingText)      BriefingText->SetText(Briefing);
	if (MapOverviewImage && MapOverview)
	{
		MapOverviewImage->SetBrushFromTexture(MapOverview, false);
	}
}

void UMC2MissionBriefingWidget::AddObjectiveLine(const FText& ObjectiveText, bool bIsPrimary)
{
	if (!ObjectivesScroll) { return; }

	UTextBlock* Line = NewObject<UTextBlock>(this);
	FString Prefix   = bIsPrimary ? TEXT("• ") : TEXT("  ○ ");
	Line->SetText(FText::FromString(Prefix + ObjectiveText.ToString()));
	ObjectivesScroll->AddChild(Line);
}

void UMC2MissionBriefingWidget::ClearObjectives()
{
	if (ObjectivesScroll) ObjectivesScroll->ClearChildren();
}

void UMC2MissionBriefingWidget::HandleLaunch() { OnLaunchPressed(); }
void UMC2MissionBriefingWidget::HandleBack()
{
	RemoveFromParent();
}
