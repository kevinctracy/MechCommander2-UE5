#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MC2MainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UMC2SessionSubsystem;

/**
 * UMC2MainMenuWidget  (WBP_MainMenu)
 * Root widget for the main menu / shell.
 * Buttons bind to C++ functions; flow handled in Blueprint.
 * Mirrors mainscreen.fit layout.
 */
UCLASS(Abstract, BlueprintType)
class MECHCOMMANDER2_API UMC2MainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Called when "New Campaign" is pressed
	UFUNCTION(BlueprintImplementableEvent, Category = "MainMenu")
	void OnNewCampaignPressed();

	// Called when "Load Game" is pressed
	UFUNCTION(BlueprintImplementableEvent, Category = "MainMenu")
	void OnLoadGamePressed();

	// Called when "Multiplayer" is pressed
	UFUNCTION(BlueprintImplementableEvent, Category = "MainMenu")
	void OnMultiplayerPressed();

	// Called when "Options" is pressed
	UFUNCTION(BlueprintImplementableEvent, Category = "MainMenu")
	void OnOptionsPressed();

	// --- Save slot query helpers (callable from Blueprint) ---

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MainMenu")
	bool HasSaveInSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "MainMenu")
	void LoadSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "MainMenu")
	void StartNewCampaign(const FString& CommanderName);

protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>    NewCampaignButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>    LoadGameButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>    MultiplayerButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>    OptionsButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>    QuitButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> VersionText;

	virtual void NativeConstruct() override;

	UFUNCTION() void HandleNewCampaign();
	UFUNCTION() void HandleLoadGame();
	UFUNCTION() void HandleMultiplayer();
	UFUNCTION() void HandleOptions();
	UFUNCTION() void HandleQuit();
};
