#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MC2GameState.generated.h"

UENUM(BlueprintType)
enum class EMC2MissionResult : uint8
{
	Playing         UMETA(DisplayName = "Playing"),
	PlayerLostBig   UMETA(DisplayName = "Player Lost (Total)"),
	PlayerLostSmall UMETA(DisplayName = "Player Lost (Partial)"),
	Draw            UMETA(DisplayName = "Draw"),
	PlayerWinSmall  UMETA(DisplayName = "Player Win (Partial)"),
	PlayerWinBig    UMETA(DisplayName = "Player Win (Total)"),
};

/** Replicated per-team data. */
USTRUCT(BlueprintType)
struct FMC2TeamState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 TeamIndex = 0;

	UPROPERTY(BlueprintReadOnly)
	FString TeamName;

	UPROPERTY(BlueprintReadOnly)
	int32 UnitsAlive = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 UnitsLost = 0;
};

/**
 * AMC2GameState
 * Replicated mission-level state: result, timers, team tallies, objective flags.
 * Mirrors MC2's scenarioResult global + mission timer system.
 */
UCLASS()
class MECHCOMMANDER2_API AMC2GameState : public AGameState
{
	GENERATED_BODY()

public:
	AMC2GameState();

	// --- Mission result ---

	UPROPERTY(ReplicatedUsing = OnRep_MissionResult, BlueprintReadOnly, Category = "Mission")
	EMC2MissionResult MissionResult = EMC2MissionResult::Playing;

	UFUNCTION(BlueprintImplementableEvent, Category = "Mission")
	void OnMissionResultChanged(EMC2MissionResult NewResult);

	// --- Scenario timers (MC2 has 8 independent timers) ---

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mission|Timers")
	TArray<float> ScenarioTimers;  // length 8, seconds elapsed

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mission|Timers")
	TArray<bool> TimerActive;

	UFUNCTION(BlueprintCallable, Category = "Mission|Timers")
	void StartTimer(int32 TimerIndex);

	UFUNCTION(BlueprintCallable, Category = "Mission|Timers")
	void StopTimer(int32 TimerIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission|Timers")
	float GetTimerValue(int32 TimerIndex) const;

	// --- Boolean mission flags (ABL eternal booleans) ---
	// 64 named flags accessible by index; mission scripts read/write these.

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mission|Flags")
	TArray<bool> MissionFlags;  // length 64

	UFUNCTION(BlueprintCallable, Category = "Mission|Flags")
	void SetMissionFlag(int32 FlagIndex, bool Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission|Flags")
	bool GetMissionFlag(int32 FlagIndex) const;

	// --- Team data ---

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mission|Teams")
	TArray<FMC2TeamState> Teams;

	UFUNCTION(BlueprintCallable, Category = "Mission|Teams")
	void RegisterTeam(int32 TeamIndex, const FString& TeamName);

	UFUNCTION(BlueprintCallable, Category = "Mission|Teams")
	void OnUnitDestroyed(int32 TeamIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission|Teams")
	bool IsTeamDefeated(int32 TeamIndex) const;

	// --- Fog of War visibility grid ---
	// Simple tile-based: 1 bit per (tile, team). Updated by USensorComponent.

	UFUNCTION(BlueprintCallable, Category = "Mission|FogOfWar")
	void SetTileVisible(int32 TileX, int32 TileY, int32 TeamIndex, bool bVisible);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission|FogOfWar")
	bool IsTileVisible(int32 TileX, int32 TileY, int32 TeamIndex) const;

	// --- Mission setup sync (mirrors MCMSG_MissionSetup) ---
	// Called by server after all Parts are spawned. Broadcasts to all clients
	// so they can dismiss loading screens and enable HUD input.

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Mission")
	void Multicast_OnMissionReady(int32 TotalUnits);

	// Blueprint event fired on each client when mission is ready
	UFUNCTION(BlueprintImplementableEvent, Category = "Mission")
	void OnMissionReady(int32 TotalUnits);

	// Whether the local client has received the mission-ready signal
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	bool bMissionReady = false;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	UFUNCTION()
	void OnRep_MissionResult();

private:
	// 256x256 tiles × 8 teams bit-packed
	static const int32 FOW_DIM = 256;
	static const int32 FOW_TEAMS = 8;
	TArray<uint8> FogOfWarBits;  // (FOW_DIM*FOW_DIM*FOW_TEAMS) / 8 bytes

	int32 FowBitIndex(int32 X, int32 Y, int32 Team) const
	{
		return (Y * FOW_DIM + X) * FOW_TEAMS + Team;
	}
};
