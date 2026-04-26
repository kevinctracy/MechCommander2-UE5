#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MC2ChooseCampaignWidget.generated.h"

/**
 * Campaign selection screen.
 * Source: mcl_choosecampaign.fit
 *
 * Shown only when multiple save slots or campaign branches exist. In MC2's single
 * linear campaign this is the new-game/continue chooser — it lets the player pick
 * a save slot to load or start a fresh campaign.
 *
 * Buttons from FIT: Cancel (ID=8), OK (implied ID=7).
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCampaignSelected, int32, SlotIndex);

USTRUCT(BlueprintType)
struct FMC2CampaignSlotInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) int32  SlotIndex    = 0;
    UPROPERTY(BlueprintReadOnly) FText  CampaignName;
    UPROPERTY(BlueprintReadOnly) FText  MissionLabel;    // e.g. "Mission 12 of 24"
    UPROPERTY(BlueprintReadOnly) FText  PlaytimeLabel;   // e.g. "14h 22m"
    UPROPERTY(BlueprintReadOnly) bool   bIsEmpty     = true;
};

UCLASS(Abstract)
class MECHCOMMANDER2_API UMC2ChooseCampaignWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Populate the slot list. Call before AddToViewport. */
    UFUNCTION(BlueprintCallable, Category = "MC2|CampaignSelect")
    void SetSlots(const TArray<FMC2CampaignSlotInfo>& Slots);

    /** Highlight a slot without confirming. */
    UFUNCTION(BlueprintCallable, Category = "MC2|CampaignSelect")
    void HighlightSlot(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "MC2|CampaignSelect")
    void HandleOK();

    UFUNCTION(BlueprintCallable, Category = "MC2|CampaignSelect")
    void HandleCancel() { RemoveFromParent(); }

    UPROPERTY(BlueprintAssignable, Category = "MC2|CampaignSelect")
    FOnCampaignSelected OnSlotChosen;

    // ── Blueprint events ──────────────────────────────────────────────────────

    UFUNCTION(BlueprintImplementableEvent, Category = "MC2|CampaignSelect")
    void OnSlotsSet(const TArray<FMC2CampaignSlotInfo>& Slots);

    UFUNCTION(BlueprintImplementableEvent, Category = "MC2|CampaignSelect")
    void OnSlotHighlighted(int32 SlotIndex);

protected:
    UPROPERTY(BlueprintReadOnly, Category = "MC2|CampaignSelect")
    int32 SelectedSlot = 0;

    UPROPERTY(BlueprintReadOnly, Category = "MC2|CampaignSelect")
    TArray<FMC2CampaignSlotInfo> CachedSlots;
};
