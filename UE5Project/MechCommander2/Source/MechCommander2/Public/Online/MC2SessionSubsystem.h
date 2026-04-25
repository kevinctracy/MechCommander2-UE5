#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MC2SessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionCreated,   bool, bSuccess, FName, SessionName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (FOnSessionDestroyed, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionJoined,    bool, bSuccess, FString, TravelURL);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (FOnSessionsFound,    bool, bSuccess);

/**
 * UMC2SessionSubsystem
 * Wraps the Online Subsystem session interface for hosting and joining
 * MC2 multiplayer matches (up to 8 players, matching MC2's max player count).
 *
 * Supports LAN (NULL OSS) and Steam (Steam OSS) automatically — the
 * underlying OSS is selected in DefaultEngine.ini.
 *
 * Usage (Blueprint):
 *   1. HostSession(MaxPlayers, bIsLAN) — creates a named session "MC2Session"
 *   2. FindSessions(bIsLAN) — populates FoundSessions; bind OnSessionsFound
 *   3. JoinSession(Index) — joins FoundSessions[Index]; bind OnSessionJoined
 *   4. DestroySession() — clean up when returning to main menu
 */
UCLASS()
class MECHCOMMANDER2_API UMC2SessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- Host ---

	UFUNCTION(BlueprintCallable, Category = "Session")
	void HostSession(int32 MaxPlayers = 4, bool bIsLAN = false);

	// --- Find ---

	UFUNCTION(BlueprintCallable, Category = "Session")
	void FindSessions(bool bIsLAN = false);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Session")
	int32 GetFoundSessionCount() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Session")
	FString GetFoundSessionDisplayName(int32 Index) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Session")
	int32 GetFoundSessionPingMs(int32 Index) const;

	// --- Join ---

	UFUNCTION(BlueprintCallable, Category = "Session")
	void JoinFoundSession(int32 Index);

	// --- Destroy ---

	UFUNCTION(BlueprintCallable, Category = "Session")
	void DestroySession();

	// --- Delegates ---

	UPROPERTY(BlueprintAssignable, Category = "Session|Events")
	FOnSessionCreated   OnSessionCreated;

	UPROPERTY(BlueprintAssignable, Category = "Session|Events")
	FOnSessionDestroyed OnSessionDestroyed;

	UPROPERTY(BlueprintAssignable, Category = "Session|Events")
	FOnSessionJoined    OnSessionJoined;

	UPROPERTY(BlueprintAssignable, Category = "Session|Events")
	FOnSessionsFound    OnSessionsFound;

private:
	static const FName SESSION_NAME;
	static const FName SETTING_MAPNAME;

	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	// OSS delegate handles
	FDelegateHandle CreateSessionHandle;
	FDelegateHandle DestroySessionHandle;
	FDelegateHandle FindSessionsHandle;
	FDelegateHandle JoinSessionHandle;

	void HandleCreateSessionComplete(FName InSessionName, bool bWasSuccessful);
	void HandleDestroySessionComplete(FName InSessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName InSessionName, EOnJoinSessionCompleteResult::Type Result);
};
