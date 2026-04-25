#include "UI/MC2UnitPortraitWidget.h"
#include "Units/MC2Mover.h"
#include "Units/MC2HealthComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/MC2CameraActor.h"
#include "EngineUtils.h"

void UMC2UnitPortraitWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PortraitButton)
		PortraitButton->OnClicked.AddDynamic(this, &UMC2UnitPortraitWidget::OnPortraitClicked);
}

void UMC2UnitPortraitWidget::BindToUnit(AMC2Mover* Unit)
{
	BoundUnit = Unit;
	Refresh();
}

void UMC2UnitPortraitWidget::Refresh()
{
	AMC2Mover* Unit = BoundUnit.Get();
	if (!Unit)
	{
		SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	SetVisibility(ESlateVisibility::Visible);

	// --- Armor bar ---
	if (ArmorBar && Unit->ArmorZones.Num() > 0)
	{
		float TotalArmor = 0.f, TotalMax = 0.f;
		for (const FMC2ArmorZone& Zone : Unit->ArmorZones)
		{
			TotalArmor += Zone.CurrentArmor;
			TotalMax   += Zone.MaxArmor;
		}
		float Pct = (TotalMax > 0.f) ? (TotalArmor / TotalMax) : 0.f;
		ArmorBar->SetPercent(Pct);
		ArmorBar->SetFillColorAndOpacity(ArmorBarColor(Pct));
	}

	// --- Heat bar ---
	if (HeatBar && Unit->MaxHeat > 0.f)
	{
		float HeatPct = Unit->CurrentHeat / Unit->MaxHeat;
		HeatBar->SetPercent(HeatPct);

		FLinearColor HeatColor = FLinearColor::LerpUsingHSV(
			FLinearColor(0.1f, 0.4f, 1.f),   // cool blue
			FLinearColor(1.f, 0.3f, 0.f),    // hot orange
			HeatPct
		);
		HeatBar->SetFillColorAndOpacity(HeatColor);
	}

	// --- Pilot name (placeholder until pilot system wired up) ---
	if (PilotNameText)
		PilotNameText->SetText(FText::FromString(Unit->GetName()));
}

void UMC2UnitPortraitWidget::OnPortraitClicked()
{
	AMC2Mover* Unit = BoundUnit.Get();
	if (!Unit) return;

	// Move camera to focus on this unit
	UWorld* World = GetWorld();
	if (!World) return;

	for (TActorIterator<AMC2CameraActor> It(World); It; ++It)
	{
		(*It)->SetActorLocation(FVector(Unit->GetActorLocation().X, Unit->GetActorLocation().Y, (*It)->GetActorLocation().Z));
		break;
	}
}

FLinearColor UMC2UnitPortraitWidget::ArmorBarColor(float ArmorPct)
{
	if (ArmorPct > 0.5f)
		return FLinearColor(0.f, 0.85f, 0.f);   // green
	if (ArmorPct > 0.25f)
		return FLinearColor(1.f, 0.75f, 0.f);   // yellow
	return FLinearColor(0.9f, 0.1f, 0.1f);      // red
}
