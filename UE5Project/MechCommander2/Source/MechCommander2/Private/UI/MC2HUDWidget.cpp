#include "UI/MC2HUDWidget.h"
#include "Units/MC2Mover.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/HorizontalBox.h"
#include "UI/MC2UnitPortraitWidget.h"

void UMC2HUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshFocusedUnitPanel(InDeltaTime);
}

void UMC2HUDWidget::SetMissionTime(float Seconds)
{
	if (!MissionClockText) return;

	int32 Minutes = FMath::FloorToInt(Seconds / 60.f);
	int32 Secs    = FMath::FloorToInt(Seconds) % 60;
	MissionClockText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Secs)));
}

void UMC2HUDWidget::SetCBills(int32 Amount)
{
	if (CBillsText)
		CBillsText->SetText(FText::AsNumber(Amount));
}

void UMC2HUDWidget::SetResourcePoints(int32 Amount)
{
	if (ResourcePointsText)
		ResourcePointsText->SetText(FText::AsNumber(Amount));
}

void UMC2HUDWidget::RefreshFocusedUnitPanel(float DeltaTime)
{
	// Refresh portrait strip widgets if unit data changed
	if (!PortraitStrip) return;

	for (int32 i = 0; i < PortraitStrip->GetChildrenCount(); ++i)
	{
		if (UMC2UnitPortraitWidget* Portrait = Cast<UMC2UnitPortraitWidget>(PortraitStrip->GetChildAt(i)))
			Portrait->Refresh();
	}
}
