#include "UI/MC2TacMapWidget.h"
#include "Units/MC2Mover.h"
#include "Mission/MC2GameMode.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void UMC2TacMapWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (CloseButton) CloseButton->OnClicked.AddDynamic(this, &UMC2TacMapWidget::HandleClose);
}

void UMC2TacMapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshBlips();
}

FVector2D UMC2TacMapWidget::WorldToMapUV(FVector WorldPos) const
{
	const float U = FMath::GetMappedRangeValueClamped(
		FVector2D(MapWorldMin.X, MapWorldMax.X), FVector2D(0.f, 1.f), WorldPos.X);
	const float V = FMath::GetMappedRangeValueClamped(
		FVector2D(MapWorldMin.Y, MapWorldMax.Y), FVector2D(0.f, 1.f), WorldPos.Y);
	return FVector2D(U, V);
}

FVector2D UMC2TacMapWidget::UVToCanvasPosition(FVector2D UV) const
{
	return UV * CanvasSize;
}

void UMC2TacMapWidget::RefreshBlips()
{
	if (!BlipCanvas) { return; }

	// Collect all visible movers from the world
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(this, AMC2Mover::StaticClass(), Found);

	TArray<AMC2Mover*> Movers;
	for (AActor* A : Found)
	{
		if (AMC2Mover* M = Cast<AMC2Mover>(A))
		{
			Movers.Add(M);
		}
	}

	// Delegate blip Widget spawning to Blueprint (it creates the coloured dot widgets
	// and positions them via CanvasPanelSlot using UVToCanvasPosition)
	OnBlipsRefreshed(Movers);
}

void UMC2TacMapWidget::CloseTacMap()
{
	RemoveFromParent();
}

void UMC2TacMapWidget::HandleClose()
{
	CloseTacMap();
}
