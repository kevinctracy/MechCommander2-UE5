#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MC2MPStatsWidget.generated.h"

/**
 * End-of-match stats screen.
 * Source: mcl_mp_stats.fit + mcl_mp_stats_entry.fit
 *
 * FIT layout:
 *   - "STATS RECAP" header text
 *   - Map name text
 *   - Tac map thumbnail image (505,166 — 138px wide)
 *   - Per-player stat rows (mcl_mp_stats_entry.fit, first at 27,127 — 468 wide)
 *   - Close button (666,536 — ID=50)
 *   - Chat toggle button (146,549)
 *
 * Per-player row (from mcl_mp_stats_entry.fit): team color, name, score, kills, losses.
 */

USTRUCT(BlueprintType)
struct FMC2MatchStatEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FString     PlayerName;
    UPROPERTY(BlueprintReadOnly) int32       TeamIndex     = 0;
    UPROPERTY(BlueprintReadOnly) FLinearColor TeamColor    = FLinearColor::White;
    UPROPERTY(BlueprintReadOnly) int32       Score         = 0;
    UPROPERTY(BlueprintReadOnly) int32       Kills         = 0;
    UPROPERTY(BlueprintReadOnly) int32       Losses        = 0;
    UPROPERTY(BlueprintReadOnly) float       DamageDone    = 0.f;
    UPROPERTY(BlueprintReadOnly) bool        bIsLocalPlayer = false;
    UPROPERTY(BlueprintReadOnly) bool        bIsWinner     = false;
};

UCLASS(Abstract)
class MECHCOMMANDER2_API UMC2MPStatsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * Populate the stats screen. Call once when the match ends.
     * MapName, TacMapTexture, and PlayerStats come from GameState.
     */
    UFUNCTION(BlueprintCallable, Category = "MC2|MPStats")
    void SetMatchResults(const FText& MapName,
                         UTexture2D* TacMapTexture,
                         const TArray<FMC2MatchStatEntry>& PlayerStats,
                         int32 WinningTeam);

    UFUNCTION(BlueprintCallable, Category = "MC2|MPStats")
    void HandleClose();

    UFUNCTION(BlueprintCallable, Category = "MC2|MPStats")
    void HandleChatToggle();

    // ── Blueprint events ──────────────────────────────────────────────────────

    UFUNCTION(BlueprintImplementableEvent, Category = "MC2|MPStats")
    void OnResultsSet(const FText& MapName,
                      UTexture2D* TacMapTexture,
                      const TArray<FMC2MatchStatEntry>& SortedStats,
                      int32 WinningTeam);

    UFUNCTION(BlueprintImplementableEvent, Category = "MC2|MPStats")
    void OnChatToggled(bool bChatVisible);

protected:
    UPROPERTY(BlueprintReadOnly, Category = "MC2|MPStats")
    bool bChatVisible = false;
};
