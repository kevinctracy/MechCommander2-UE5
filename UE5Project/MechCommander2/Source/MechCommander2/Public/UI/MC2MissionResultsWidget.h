#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MC2GameState.h"
#include "MC2MissionResultsWidget.generated.h"

class UTextBlock;
class UButton;
class UImage;
class UVerticalBox;

/**
 * UMC2MissionResultsWidget  (WBP_MissionResults)
 * Post-mission debrief screen.
 * Shows: result banner (Win/Loss/Draw), kills/losses, objectives,
 * C-Bills earned, XP gained, salvage available.
 *
 * Populated by AMC2GameMode after mission end via OnMissionResult event.
 */
UCLASS(Abstract, BlueprintType)
class MECHCOMMANDER2_API UMC2MissionResultsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Call once after mission ends with all relevant stats
	UFUNCTION(BlueprintCallable, Category = "Results")
	void SetResults(
		EMC2MissionResult Result,
		int32 EnemyKills,
		int32 FriendlyLosses,
		int32 CBillsEarned,
		int32 XPEarned
	);

	UFUNCTION(BlueprintCallable, Category = "Results")
	void AddObjectiveResult(const FText& ObjectiveText, bool bCompleted);

	// Fires when player accepts results and wants to go to salvage or continue
	UFUNCTION(BlueprintImplementableEvent, Category = "Results")
	void OnContinuePressed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Results")
	void OnRetryPressed();

protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock>   ResultBannerText;   // "VICTORY" / "DEFEAT" / "DRAW"
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage>       ResultBannerImage;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock>   KillsText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock>   LossesText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock>   CBillsEarnedText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock>   XPEarnedText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UVerticalBox> ObjectiveResultsList;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>      ContinueButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>      RetryButton;

	virtual void NativeConstruct() override;

	UFUNCTION() void HandleContinue();
	UFUNCTION() void HandleRetry();

private:
	static FText ResultToText(EMC2MissionResult Result);
};
