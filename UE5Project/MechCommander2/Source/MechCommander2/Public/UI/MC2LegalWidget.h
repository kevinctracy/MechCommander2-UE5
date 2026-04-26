#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MC2LegalWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLegalAccepted);

/**
 * EULA / legal info screen shown on first launch.
 * Source: mcl_dialoglegal.fit
 *
 * FIT layout:
 *   - Header text (TextID=26250, Agency FB 17, orange 0xFFFF8A00) at (126,61)
 *   - Scrollable legal body text (TextID=26252, Agency FB 11, amber 0xFFC66600)
 *     at (133,115) — 540x346 area
 *   - Single "I Accept" button (ID=50) at (666,536)
 *   - Enter anim: Y slides from -600→0 in 0.23s
 *   - Leave anim: Y slides from 0→-600 in 0.23s, then RemoveFromParent
 *
 * WBP: play EnterAnim on construct; "I Accept" button calls HandleAccept.
 */
UCLASS(Abstract)
class MECHCOMMANDER2_API UMC2LegalWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Broadcast when the user clicks "I Accept". Listen in GameInstance to persist the flag. */
    UPROPERTY(BlueprintAssignable, Category = "MC2|Legal")
    FOnLegalAccepted OnLegalAccepted;

    /**
     * Initialise the widget with localised text. Called by the owning GameMode/HUD before
     * adding to viewport so WBP can bind to OnLegalDataSet before the widget is visible.
     *
     * @param HeaderText  Title bar string (localised "Legal Information" equivalent).
     * @param BodyText    Full EULA body (may be long; WBP should put it in a ScrollBox).
     * @param AcceptLabel Label for the single accept button (localised "I Accept").
     */
    UFUNCTION(BlueprintCallable, Category = "MC2|Legal")
    void SetLegalText(const FText& HeaderText, const FText& BodyText, const FText& AcceptLabel);

    /**
     * Called by the "I Accept" button in WBP.
     * Broadcasts OnLegalAccepted, then fires OnPlayLeaveAnim so WBP can animate out.
     * Actual RemoveFromParent happens in DoRemove (called by WBP at animation end).
     */
    UFUNCTION(BlueprintCallable, Category = "MC2|Legal")
    void HandleAccept();

    /**
     * Called by WBP when the leave animation finishes (via "On Animation Finished" binding).
     * Removes the widget from the viewport.
     */
    UFUNCTION(BlueprintCallable, Category = "MC2|Legal")
    void DoRemove();

    // ── Blueprint events ──────────────────────────────────────────────────────

    /** WBP overrides to populate header, body ScrollBox, and accept button label. */
    UFUNCTION(BlueprintImplementableEvent, Category = "MC2|Legal")
    void OnLegalDataSet(const FText& HeaderText, const FText& BodyText, const FText& AcceptLabel);

    /** WBP overrides to play the leave slide animation (Y: 0→-600 in 0.23 s). */
    UFUNCTION(BlueprintImplementableEvent, Category = "MC2|Legal")
    void OnPlayLeaveAnim();

protected:
    virtual void NativeConstruct() override;

private:
    bool bAccepted = false;
};
