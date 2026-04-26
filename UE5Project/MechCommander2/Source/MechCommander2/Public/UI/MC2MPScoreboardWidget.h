#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MC2MPScoreboardWidget.generated.h"

/**
 * Multiplayer scoreboard widget — shown mid-match (Tab key) and at end of match.
 * Source: mcl_mp_scoreboard.fit + mcl_mp_stats_entry.fit (per-player row)
 *
 * FIT layout per player row (mcl_mp_scoreboard.fit):
 *   - Color rect   (10×18) — team color indicator
 *   - Team text    (17×18) — team number/name
 *   - Player name  (129×18)
 *   - Score        (70×18)
 *   - Kills        (70×18)
 *   - Losses       (70×18)
 */

USTRUCT(BlueprintType)
struct FMC2ScoreboardEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FString PlayerName;
    UPROPERTY(BlueprintReadOnly) int32   TeamIndex    = 0;
    UPROPERTY(BlueprintReadOnly) FLinearColor TeamColor = FLinearColor::White;
    UPROPERTY(BlueprintReadOnly) int32   Score        = 0;
    UPROPERTY(BlueprintReadOnly) int32   Kills        = 0;
    UPROPERTY(BlueprintReadOnly) int32   Losses       = 0;
    UPROPERTY(BlueprintReadOnly) bool    bIsLocalPlayer = false;
    UPROPERTY(BlueprintReadOnly) bool    bIsReady     = false;
};

UCLASS(Abstract)
class MECHCOMMANDER2_API UMC2MPScoreboardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Refresh the full scoreboard. Call from GameState replication callbacks. */
    UFUNCTION(BlueprintCallable, Category = "MC2|Scoreboard")
    void RefreshScoreboard(const TArray<FMC2ScoreboardEntry>& Entries);

    /** Update a single player row (e.g. on OnRep_Score). */
    UFUNCTION(BlueprintCallable, Category = "MC2|Scoreboard")
    void UpdatePlayerEntry(const FMC2ScoreboardEntry& Entry);

    /** Sort entries: by team first, then by score descending. */
    UFUNCTION(BlueprintCallable, Category = "MC2|Scoreboard")
    static TArray<FMC2ScoreboardEntry> SortEntries(TArray<FMC2ScoreboardEntry> Entries);

    /** Show/hide (Tab toggles visibility mid-match). */
    UFUNCTION(BlueprintCallable, Category = "MC2|Scoreboard")
    void ToggleVisibility();

    UFUNCTION(BlueprintCallable, Category = "MC2|Scoreboard")
    void Close() { RemoveFromParent(); }

    // ── Blueprint events ──────────────────────────────────────────────────────

    UFUNCTION(BlueprintImplementableEvent, Category = "MC2|Scoreboard")
    void OnScoreboardRefreshed(const TArray<FMC2ScoreboardEntry>& SortedEntries);

    UFUNCTION(BlueprintImplementableEvent, Category = "MC2|Scoreboard")
    void OnPlayerEntryUpdated(const FMC2ScoreboardEntry& Entry);

protected:
    UPROPERTY(BlueprintReadOnly, Category = "MC2|Scoreboard")
    TArray<FMC2ScoreboardEntry> CachedEntries;
};
