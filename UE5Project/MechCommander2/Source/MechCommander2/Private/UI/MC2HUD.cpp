#include "UI/MC2HUD.h"
#include "UI/MC2HUDWidget.h"
#include "Units/MC2Mover.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Canvas.h"

const FLinearColor AMC2HUD::BOX_COLOR = FLinearColor(0.f, 1.f, 0.f, 0.8f);

AMC2HUD::AMC2HUD()
{
}

void AMC2HUD::BeginPlay()
{
	Super::BeginPlay();

	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UMC2HUDWidget>(GetOwningPlayerController(), HUDWidgetClass);
		if (HUDWidget)
			HUDWidget->AddToViewport();
	}
}

void AMC2HUD::DrawHUD()
{
	Super::DrawHUD();

	if (!bDrawingSelection)
		return;

	// Draw rubber-band selection rectangle
	float X1 = FMath::Min(SelectionAnchor.X, SelectionCurrent.X);
	float Y1 = FMath::Min(SelectionAnchor.Y, SelectionCurrent.Y);
	float X2 = FMath::Max(SelectionAnchor.X, SelectionCurrent.X);
	float Y2 = FMath::Max(SelectionAnchor.Y, SelectionCurrent.Y);

	DrawRect(FLinearColor(0.f, 1.f, 0.f, 0.05f), X1, Y1, X2 - X1, Y2 - Y1);
	DrawLine(X1, Y1, X2, Y1, BOX_COLOR, BOX_THICKNESS);
	DrawLine(X2, Y1, X2, Y2, BOX_COLOR, BOX_THICKNESS);
	DrawLine(X2, Y2, X1, Y2, BOX_COLOR, BOX_THICKNESS);
	DrawLine(X1, Y2, X1, Y1, BOX_COLOR, BOX_THICKNESS);
}

void AMC2HUD::BeginSelectionBox(const FVector2D& AnchorScreen)
{
	bDrawingSelection  = true;
	SelectionAnchor    = AnchorScreen;
	SelectionCurrent   = AnchorScreen;
}

void AMC2HUD::UpdateSelectionBox(const FVector2D& CurrentScreen)
{
	SelectionCurrent = CurrentScreen;
}

void AMC2HUD::EndSelectionBox()
{
	bDrawingSelection = false;
}

FBox2D AMC2HUD::GetSelectionRect() const
{
	return FBox2D(
		FVector2D(FMath::Min(SelectionAnchor.X, SelectionCurrent.X),
		          FMath::Min(SelectionAnchor.Y, SelectionCurrent.Y)),
		FVector2D(FMath::Max(SelectionAnchor.X, SelectionCurrent.X),
		          FMath::Max(SelectionAnchor.Y, SelectionCurrent.Y))
	);
}

void AMC2HUD::SetSelectedUnits(const TArray<AMC2Mover*>& Units)
{
	if (HUDWidget)
		HUDWidget->OnSelectionChanged(Units);
}
