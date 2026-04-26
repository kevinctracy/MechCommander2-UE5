#include "UI/MC2MPStatsWidget.h"

void UMC2MPStatsWidget::SetMatchResults(const FText& MapName,
                                        UTexture2D* TacMapTexture,
                                        const TArray<FMC2MatchStatEntry>& PlayerStats,
                                        int32 WinningTeam)
{
    // Sort: winners first, then by score descending within each team
    TArray<FMC2MatchStatEntry> Sorted = PlayerStats;
    Sorted.StableSort([WinningTeam](const FMC2MatchStatEntry& A, const FMC2MatchStatEntry& B)
    {
        const bool AWin = (A.TeamIndex == WinningTeam);
        const bool BWin = (B.TeamIndex == WinningTeam);
        if (AWin != BWin) return AWin > BWin;
        if (A.TeamIndex != B.TeamIndex) return A.TeamIndex < B.TeamIndex;
        return A.Score > B.Score;
    });

    OnResultsSet(MapName, TacMapTexture, Sorted, WinningTeam);
}

void UMC2MPStatsWidget::HandleClose()
{
    RemoveFromParent();
}

void UMC2MPStatsWidget::HandleChatToggle()
{
    bChatVisible = !bChatVisible;
    OnChatToggled(bChatVisible);
}
