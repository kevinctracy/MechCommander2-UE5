#include "UI/MC2MinimapWidget.h"
#include "Units/MC2Mover.h"
#include "MC2GameState.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

void UMC2MinimapWidget::SetMapBounds(const FVector2D& Origin, const FVector2D& Extent)
{
	MapOrigin = Origin;
	MapExtent = Extent;
}

void UMC2MinimapWidget::SetTerrainTexture(UTexture2D* Texture)
{
	if (TerrainImage && Texture)
		TerrainImage->SetBrushFromTexture(Texture);
}

FVector2D UMC2MinimapWidget::WorldToMinimap(const FVector& WorldPos) const
{
	FVector2D UV;
	UV.X = (WorldPos.X - MapOrigin.X) / MapExtent.X;
	UV.Y = (WorldPos.Y - MapOrigin.Y) / MapExtent.Y;
	return FVector2D(FMath::Clamp(UV.X, 0.f, 1.f), FMath::Clamp(UV.Y, 0.f, 1.f));
}

void UMC2MinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateBlips(MyGeometry);
}

void UMC2MinimapWidget::UpdateBlips(const FGeometry& Geom)
{
	if (!BlipCanvas) return;

	UWorld* World = GetWorld();
	if (!World) return;

	const FVector2D PanelSize = Geom.GetLocalSize();

	int32 BlipIndex = 0;
	for (TActorIterator<AMC2Mover> It(World); It; ++It)
	{
		AMC2Mover* Unit = *It;
		if (!Unit || Unit->bIsDestroyed) continue;

		FVector2D UV  = WorldToMinimap(Unit->GetActorLocation());
		FVector2D Pos = UV * PanelSize;

		// Reuse existing blip widgets or create new ones (Blueprint child class handles visuals)
		if (BlipIndex < BlipCanvas->GetChildrenCount())
		{
			UWidget* Blip = BlipCanvas->GetChildAt(BlipIndex);
			if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Blip->Slot))
				Slot->SetPosition(Pos - FVector2D(3.f, 3.f));  // center 6×6 blip
		}
		BlipIndex++;
	}

	// Hide excess blips
	for (int32 i = BlipIndex; i < BlipCanvas->GetChildrenCount(); ++i)
		BlipCanvas->GetChildAt(i)->SetVisibility(ESlateVisibility::Hidden);
}

FReply UMC2MinimapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FVector2D LocalPos = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	FVector2D PanelSize = InGeometry.GetLocalSize();

	if (PanelSize.X > 0.f && PanelSize.Y > 0.f)
	{
		FVector2D NormalizedPos(LocalPos.X / PanelSize.X, LocalPos.Y / PanelSize.Y);
		OnMinimapClicked(NormalizedPos);
	}

	return FReply::Handled();
}
