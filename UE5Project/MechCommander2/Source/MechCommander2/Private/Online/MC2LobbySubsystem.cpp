#include "Online/MC2LobbySubsystem.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"

// ---------------------------------------------------------------------------
// Map / team settings
// ---------------------------------------------------------------------------

void UMC2LobbySubsystem::SetSelectedMap(FName MissionID)
{
	SelectedMissionID = MissionID;
	OnLobbyStateChanged.Broadcast();
}

void UMC2LobbySubsystem::SetTeamSize(int32 Size)
{
	TeamSize = FMath::Clamp(Size, 1, 4);
	OnLobbyStateChanged.Broadcast();
}

// ---------------------------------------------------------------------------
// Local player state
// ---------------------------------------------------------------------------

void UMC2LobbySubsystem::SetLocalPlayerReady(bool bReady)
{
	// Find local player's entry and update bReady.
	// The actual ID is set when RegisterPlayer is called from GameMode.
	// For now, update the first entry with a matching controller (handled in BP).
	for (FMC2LobbyPlayer& P : LobbyPlayers)
	{
		if (P.PlayerID.IsValid() && P.PlayerID.IsLocalNetIdEqual(P.PlayerID))
		{
			P.bReady = bReady;
			OnLobbyStateChanged.Broadcast();
			return;
		}
	}
}

void UMC2LobbySubsystem::SetLocalPlayerLance(const TArray<FName>& MechChassisIDs)
{
	for (FMC2LobbyPlayer& P : LobbyPlayers)
	{
		if (P.PlayerID.IsValid() && P.PlayerID.IsLocalNetIdEqual(P.PlayerID))
		{
			P.LanceMechs = MechChassisIDs;
			P.LanceMechs.SetNum(FMath::Min(MechChassisIDs.Num(), 4));
			OnLobbyStateChanged.Broadcast();
			return;
		}
	}
}

void UMC2LobbySubsystem::SetLocalPlayerTeam(int32 NewTeamIndex)
{
	for (FMC2LobbyPlayer& P : LobbyPlayers)
	{
		if (P.PlayerID.IsValid() && P.PlayerID.IsLocalNetIdEqual(P.PlayerID))
		{
			P.TeamIndex = NewTeamIndex;
			OnLobbyStateChanged.Broadcast();
			return;
		}
	}
}

// ---------------------------------------------------------------------------
// Roster management
// ---------------------------------------------------------------------------

void UMC2LobbySubsystem::RegisterPlayer(const FUniqueNetIdRepl& PlayerID, const FString& Name)
{
	if (FindPlayerIndex(PlayerID) != INDEX_NONE) return;

	FMC2LobbyPlayer Entry;
	Entry.PlayerID    = PlayerID;
	Entry.DisplayName = Name;
	Entry.TeamIndex   = LobbyPlayers.Num() % 2;  // alternate teams by join order
	Entry.LanceMechs.Init(NAME_None, 4);
	LobbyPlayers.Add(Entry);
	OnLobbyStateChanged.Broadcast();
}

void UMC2LobbySubsystem::UnregisterPlayer(const FUniqueNetIdRepl& PlayerID)
{
	const int32 Idx = FindPlayerIndex(PlayerID);
	if (Idx != INDEX_NONE)
	{
		LobbyPlayers.RemoveAt(Idx);
		OnLobbyStateChanged.Broadcast();
	}
}

void UMC2LobbySubsystem::UpdatePlayerState(const FMC2LobbyPlayer& UpdatedState)
{
	const int32 Idx = FindPlayerIndex(UpdatedState.PlayerID);
	if (Idx != INDEX_NONE)
	{
		LobbyPlayers[Idx] = UpdatedState;
		OnLobbyStateChanged.Broadcast();
	}
}

int32 UMC2LobbySubsystem::GetReadyCount() const
{
	int32 Count = 0;
	for (const FMC2LobbyPlayer& P : LobbyPlayers)
		if (P.bReady) ++Count;
	return Count;
}

bool UMC2LobbySubsystem::AreAllPlayersReady() const
{
	if (LobbyPlayers.IsEmpty()) return false;
	for (const FMC2LobbyPlayer& P : LobbyPlayers)
		if (!P.bReady) return false;
	return true;
}

// ---------------------------------------------------------------------------
// Match start
// ---------------------------------------------------------------------------

void UMC2LobbySubsystem::StartMatch()
{
	if (SelectedMissionID.IsNone()) return;

	OnMatchStarting.Broadcast();

	// Server travel: level name matches convention L_MC2_01_MissionName etc.
	// The mission ID (e.g. "MC2_01") is the level short name.
	if (UWorld* World = GetGameInstance()->GetWorld())
	{
		const FString LevelName = SelectedMissionID.ToString();
		World->ServerTravel(LevelName + TEXT("?listen"), false);
	}
}

// ---------------------------------------------------------------------------

int32 UMC2LobbySubsystem::FindPlayerIndex(const FUniqueNetIdRepl& PlayerID) const
{
	for (int32 i = 0; i < LobbyPlayers.Num(); ++i)
		if (LobbyPlayers[i].PlayerID == PlayerID) return i;
	return INDEX_NONE;
}
