#include "UI/MC2ChooseCampaignWidget.h"
#include "Campaign/MC2LogisticsSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UMC2ChooseCampaignWidget::SetSlots(const TArray<FMC2CampaignSlotInfo>& Slots)
{
    CachedSlots = Slots;
    SelectedSlot = 0;
    // Select first non-empty slot by default
    for (const FMC2CampaignSlotInfo& S : Slots)
    {
        if (!S.bIsEmpty)
        {
            SelectedSlot = S.SlotIndex;
            break;
        }
    }
    OnSlotsSet(Slots);
    OnSlotHighlighted(SelectedSlot);
}

void UMC2ChooseCampaignWidget::HighlightSlot(int32 SlotIndex)
{
    SelectedSlot = SlotIndex;
    OnSlotHighlighted(SlotIndex);
}

void UMC2ChooseCampaignWidget::HandleOK()
{
    OnSlotChosen.Broadcast(SelectedSlot);
    RemoveFromParent();
}
