#include "UI/MC2MPScoreboardWidget.h"

void UMC2MPScoreboardWidget::RefreshScoreboard(const TArray<FMC2ScoreboardEntry>& Entries)
{
    CachedEntries = SortEntries(Entries);
    OnScoreboardRefreshed(CachedEntries);
}

void UMC2MPScoreboardWidget::UpdatePlayerEntry(const FMC2ScoreboardEntry& Entry)
{
    for (FMC2ScoreboardEntry& Cached : CachedEntries)
    {
        if (Cached.PlayerName == Entry.PlayerName)
        {
            Cached = Entry;
            break;
        }
    }
    CachedEntries = SortEntries(CachedEntries);
    OnPlayerEntryUpdated(Entry);
}

TArray<FMC2ScoreboardEntry> UMC2MPScoreboardWidget::SortEntries(
    TArray<FMC2ScoreboardEntry> Entries)
{
    Entries.StableSort([](const FMC2ScoreboardEntry& A, const FMC2ScoreboardEntry& B)
    {
        if (A.TeamIndex != B.TeamIndex) return A.TeamIndex < B.TeamIndex;
        return A.Score > B.Score;
    });
    return Entries;
}

void UMC2MPScoreboardWidget::ToggleVisibility()
{
    SetVisibility(GetVisibility() == ESlateVisibility::Visible
        ? ESlateVisibility::Hidden
        : ESlateVisibility::Visible);
}
