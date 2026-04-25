#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MC2MissionBriefingWidget.generated.h"

class UTextBlock;
class URichTextBlock;
class UButton;
class UImage;
class UScrollBox;

/**
 * UMC2MissionBriefingWidget  (WBP_MissionBriefing)
 * Pre-mission screen shown after lance selection.
 * Displays mission title, map overview, objective list, and narrative briefing text.
 * "Launch" button transitions to the mission level.
 *
 * Data is provided by a DA_MissionBriefing Data Asset set on the GameMode Blueprint.
 */
UCLASS(Abstract, BlueprintType)
class MECHCOMMANDER2_API UMC2MissionBriefingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Briefing")
	void SetMissionData(const FText& Title, const FText& BriefingText, UTexture2D* MapOverview);

	UFUNCTION(BlueprintCallable, Category = "Briefing")
	void AddObjectiveLine(const FText& ObjectiveText, bool bIsPrimary);

	UFUNCTION(BlueprintCallable, Category = "Briefing")
	void ClearObjectives();

	UFUNCTION(BlueprintImplementableEvent, Category = "Briefing")
	void OnLaunchPressed();

protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock>     MissionTitleText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<URichTextBlock> BriefingText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage>         MapOverviewImage;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UScrollBox>     ObjectivesScroll;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>        LaunchButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>        BackButton;

	virtual void NativeConstruct() override;

	UFUNCTION() void HandleLaunch();
	UFUNCTION() void HandleBack();
};
