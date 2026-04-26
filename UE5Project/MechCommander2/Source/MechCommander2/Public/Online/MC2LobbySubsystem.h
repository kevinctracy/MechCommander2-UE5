#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MC2LobbySubsystem.generated.h"

/**
 * FMC2LobbyPlayer
 * Per-player state in the pre-match lobby.
 * Maps to MCL_MP_*.fit lobby screen data (player name, team, lance mechs).
 */
USTRUCT(BlueprintType)
struct FMC2LobbyPlayer
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FUniqueNetIdRepl PlayerID;
	UPROPERTY(BlueprintReadOnly) FString         DisplayName;
	UPROPERTY(BlueprintReadOnly) int32           TeamIndex     = 0;   // 0=Mercenaries, 1=Davion, etc.
	UPROPERTY(BlueprintReadOnly) bool            bReady        = false;

	// Up to 4 mech chassis IDs the player is bringing (from their save game roster).
	// Empty FName = no mech in that lance slot.
	UPROPERTY(BlueprintReadOnly) TArray<FName>   LanceMechs;         // max 4
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMatchStarting);

/**
 * UMC2LobbySubsystem
 * Game-specific lobby state for multiplayer matches.
 * Sits on top of UMC2SessionSubsystem (network) and drives WBP_MPLobby.
 *
 * Host calls SetSelectedMap / SetTeamSize then StartMatch when all players ready.
 * Clients call SetReady / SetLanceMechs — replicated via PlayerState (blueprint).
 *
 * Mirrors MC2's MCL_MP_*.fit lobby screens:
 *   MCL_MP_GN — game name/map select (host only)
 *   MCL_MP_PT — player team select
 *   MCL_MP_LC — lance mech select (per player)
 */
UCLASS()
class MECHCOMMANDER2_API UMC2LobbySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// --- Map / match settings (host only) ---

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void SetSelectedMap(FName MissionID);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lobby")
	FName GetSelectedMap() const { return SelectedMissionID; }

	// Max players per team (1-4). Total players = TeamSize × NumTeams (2 teams always).
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void SetTeamSize(int32 Size);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lobby")
	int32 GetTeamSize() const { return TeamSize; }

	// --- Player state ---

	// Call from local player when they choose their lance and confirm ready.
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void SetLocalPlayerReady(bool bReady);

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void SetLocalPlayerLance(const TArray<FName>& MechChassisIDs);

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void SetLocalPlayerTeam(int32 NewTeamIndex);

	// --- Lobby roster ---

	// Called by GameMode when a player connects/disconnects.
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void RegisterPlayer(const FUniqueNetIdRepl& PlayerID, const FString& Name);

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void UnregisterPlayer(const FUniqueNetIdRepl& PlayerID);

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void UpdatePlayerState(const FMC2LobbyPlayer& UpdatedState);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lobby")
	const TArray<FMC2LobbyPlayer>& GetLobbyPlayers() const { return LobbyPlayers; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lobby")
	int32 GetReadyCount() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lobby")
	bool AreAllPlayersReady() const;

	// --- Match start (host only) ---

	// Travels all clients to the selected mission map.
	// GameMode on the map level handles the full mission setup.
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void StartMatch();

	// --- Events ---

	UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
	FOnLobbyStateChanged OnLobbyStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
	FOnMatchStarting OnMatchStarting;

private:
	UPROPERTY()
	TArray<FMC2LobbyPlayer> LobbyPlayers;

	FName SelectedMissionID  = NAME_None;
	int32 TeamSize           = 4;

	int32 FindPlayerIndex(const FUniqueNetIdRepl& PlayerID) const;
};
