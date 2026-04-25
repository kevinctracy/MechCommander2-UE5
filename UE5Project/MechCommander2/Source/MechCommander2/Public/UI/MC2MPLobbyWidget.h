#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MC2MPLobbyWidget.generated.h"

class UListView;
class UTextBlock;
class UButton;
class UEditableTextBox;
class USpinBox;
class UCheckBox;
class UMC2SessionSubsystem;

/**
 * UMC2MPLobbyWidget  (WBP_MPLobby)
 * Multiplayer session browser and host/join lobby.
 * Mirrors MCL_MP_*.fit screens.
 *
 * Flow:
 *   Host tab:  configure map, player count, LAN/online → HostSession
 *   Find tab:  search for sessions → list results → JoinFoundSession
 *   Lobby:     show connected players → host presses Launch → ServerTravel
 */
UCLASS(Abstract, BlueprintType)
class MECHCOMMANDER2_API UMC2MPLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- Host ---
	UFUNCTION(BlueprintCallable, Category = "Lobby|Host")
	void HostSession();

	// --- Find & Join ---
	UFUNCTION(BlueprintCallable, Category = "Lobby|Find")
	void FindSessions();

	UFUNCTION(BlueprintCallable, Category = "Lobby|Find")
	void JoinSelectedSession();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lobby|Find")
	int32 GetSelectedSessionIndex() const { return SelectedSessionIndex; }

	// --- Lobby state ---
	UFUNCTION(BlueprintImplementableEvent, Category = "Lobby")
	void OnSessionCreated(bool bSuccess);

	UFUNCTION(BlueprintImplementableEvent, Category = "Lobby")
	void OnSessionsFound(bool bSuccess, int32 NumFound);

	UFUNCTION(BlueprintImplementableEvent, Category = "Lobby")
	void OnJoinedSession(bool bSuccess);

	UFUNCTION(BlueprintImplementableEvent, Category = "Lobby")
	void OnPlayerListUpdated();

protected:
	// Host tab
	UPROPERTY(meta=(BindWidget)) TObjectPtr<USpinBox>          MaxPlayersSpinner;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UCheckBox>         LANCheck;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>           HostButton;

	// Find tab
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UListView>         SessionList;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>           FindButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>           JoinButton;

	// Lobby
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UListView>         PlayerList;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock>        StatusText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>           LaunchButton;  // host only
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>           BackButton;

	virtual void NativeConstruct() override;

	UFUNCTION() void HandleHost();
	UFUNCTION() void HandleFind();
	UFUNCTION() void HandleJoin();
	UFUNCTION() void HandleLaunch();
	UFUNCTION() void HandleBack();

private:
	int32 SelectedSessionIndex = -1;

	UFUNCTION()
	void OnSessionCreatedInternal(bool bSuccess, FName SessionName);
	UFUNCTION()
	void OnSessionsFoundInternal(bool bSuccess);
	UFUNCTION()
	void OnJoinedInternal(bool bSuccess, FString TravelURL);
};
