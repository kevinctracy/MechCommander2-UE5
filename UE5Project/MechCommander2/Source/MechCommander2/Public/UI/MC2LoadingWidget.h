#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MC2LoadingWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UImage;

/**
 * UMC2LoadingWidget  (WBP_Loading)
 * Full-screen loading overlay shown during level transitions.
 * Displays mission name, briefing excerpt, and async load progress.
 * Matches mcl_loadingscreen.fit aesthetics.
 *
 * Usage: AddToViewport(10) before AsyncLoadLevel, update Progress each tick,
 * RemoveFromParent() when level is ready.
 */
UCLASS(Abstract, BlueprintType)
class MECHCOMMANDER2_API UMC2LoadingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Loading")
	void SetMissionInfo(const FText& MissionName, const FText& BriefingExcerpt, UTexture2D* LoadingImage);

	UFUNCTION(BlueprintCallable, Category = "Loading")
	void SetProgress(float Progress);   // 0.0 – 1.0

	UFUNCTION(BlueprintCallable, Category = "Loading")
	void SetTip(const FText& TipText);

protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock>   MissionNameText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock>   BriefingText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock>   TipText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> LoadingBar;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage>       LoadingImage;
};
