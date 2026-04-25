#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MC2MinimapWidget.generated.h"

class AMC2Mover;
class AMC2GameState;
class UImage;
class UCanvasPanel;

/**
 * UMC2MinimapWidget  (WBP_Minimap)
 * Renders a top-down tactical minimap.
 *
 * Approach:
 *   - Background: static terrain overview texture (set from level Blueprint)
 *   - Unit blips: UCanvasPanel children positioned each tick via NativeTick
 *   - Fog of war: semi-transparent overlay driven by AMC2GameState::FogOfWarBits
 *   - Camera frustum: white rectangle showing the current view area
 *
 * Map coordinate system:
 *   World position → minimap UV = (WorldXY - MapOrigin) / MapExtent
 */
UCLASS(Abstract, BlueprintType)
class MECHCOMMANDER2_API UMC2MinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Set once from the level Blueprint after terrain is loaded
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetMapBounds(const FVector2D& Origin, const FVector2D& Extent);

	// Background overview texture (set from level Blueprint or DA_Mission)
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetTerrainTexture(UTexture2D* Texture);

	// Called from AMC2PlayerController when the player clicks the minimap
	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap")
	void OnMinimapClicked(const FVector2D& NormalizedPos);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage>        TerrainImage;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UCanvasPanel>  BlipCanvas;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage>        FoWOverlay;     // dynamic texture updated each frame

	// Team colors for blips
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minimap")
	FLinearColor PlayerTeamColor  = FLinearColor(0.f, 0.8f, 0.f, 1.f);   // green
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minimap")
	FLinearColor EnemyTeamColor   = FLinearColor(0.9f, 0.1f, 0.1f, 1.f); // red
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minimap")
	FLinearColor NeutralTeamColor = FLinearColor(0.8f, 0.8f, 0.8f, 1.f); // grey

private:
	FVector2D MapOrigin = FVector2D::ZeroVector;
	FVector2D MapExtent = FVector2D(15000.f, 15000.f);

	// Convert world XY to minimap panel [0,1] UV
	FVector2D WorldToMinimap(const FVector& WorldPos) const;

	void UpdateBlips(const FGeometry& Geom);
	void UpdateFoWOverlay();
};
