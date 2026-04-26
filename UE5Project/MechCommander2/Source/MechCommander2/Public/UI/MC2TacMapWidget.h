#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MC2TacMapWidget.generated.h"

class UImage;
class UButton;
class UCanvasPanel;
class AMC2Mover;
class AMC2GameState;

/**
 * UMC2TacMapWidget  (WBP_TacMap)
 * Full-screen tactical map overlay (Tab key).
 * Shows unit positions as coloured blips, nav markers, and fog of war overlay.
 *
 * The map texture is a render target updated by BP_MinimapCapture (top-down SceneCaptureComponent2D).
 * Unit blips are Widget Components positioned in the CanvasPanel by world→UV projection.
 *
 * Maps to MC2's tacMapInterface full-screen mode.
 */
UCLASS(Abstract, BlueprintType)
class MECHCOMMANDER2_API UMC2TacMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// World bounds of the map (set from Landscape bounds at BeginPlay)
	UPROPERTY(BlueprintReadWrite, Category = "TacMap")
	FVector2D MapWorldMin = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "TacMap")
	FVector2D MapWorldMax = FVector2D(120000.f, 120000.f); // 1200m × 1200m default

	// Called every frame while the tac map is visible to refresh blip positions
	UFUNCTION(BlueprintCallable, Category = "TacMap")
	void RefreshBlips();

	// Project a world XY position to UV [0,1] space on the map canvas
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TacMap")
	FVector2D WorldToMapUV(FVector WorldPos) const;

	// Project UV to canvas pixel position (for widget placement)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TacMap")
	FVector2D UVToCanvasPosition(FVector2D UV) const;

	// Close the tac map (remove from viewport)
	UFUNCTION(BlueprintCallable, Category = "TacMap")
	void CloseTacMap();

	UFUNCTION(BlueprintImplementableEvent, Category = "TacMap")
	void OnBlipsRefreshed(const TArray<AMC2Mover*>& VisibleUnits);

protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage>       MapImage;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UCanvasPanel> BlipCanvas;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>      CloseButton;

	// Size of the BlipCanvas in pixels (set in the WBP editor)
	UPROPERTY(EditDefaultsOnly, Category = "TacMap")
	FVector2D CanvasSize = FVector2D(512.f, 512.f);

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION() void HandleClose();
};
