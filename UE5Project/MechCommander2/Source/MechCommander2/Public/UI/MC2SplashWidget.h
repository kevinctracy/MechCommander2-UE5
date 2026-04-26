#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MC2SplashWidget.generated.h"

/**
 * Intro splash / logo screen.
 * Source: mcl_splashscreenintro.fit
 *
 * FIT describes three animated objects:
 *   AnimObject0 — full-screen black rect, fades OUT at t=8→11s (alpha 1→0)
 *   AnimObject1 — Microsoft logo image (mcl_splashscreenintro_1.tga),
 *                 fades IN at t=3.5→5.5s, fades OUT at t=8→11s
 *   AnimObject2 — full-screen white rect, fades from alpha=1 to 0 over first 3s
 *                 (white flash into black → logo reveal)
 *
 * Total animation: ~12 seconds, then auto-advance to main menu.
 * In WBP: use UWidgetAnimation with a Play On Construct animation lasting 12s,
 * with a "Finished" binding that calls AdvanceToMainMenu.
 *
 * Players can skip by pressing any key (bound to HandleSkip).
 */

UCLASS(Abstract)
class MECHCOMMANDER2_API UMC2SplashWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Total splash duration before auto-advancing. From FIT: 12.0 seconds. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MC2|Splash")
    float SplashDuration = 12.0f;

    /**
     * Called by the "any key" input binding or by animation completion.
     * Opens the main menu level if not already there.
     */
    UFUNCTION(BlueprintCallable, Category = "MC2|Splash")
    void HandleSkip();

    /** Start the splash sequence. Plays logo animation and starts the auto-skip timer. */
    UFUNCTION(BlueprintCallable, Category = "MC2|Splash")
    void StartSplash();

    // ── Blueprint events ──────────────────────────────────────────────────────

    /** Fires when the splash should advance. WBP overrides to open the main menu. */
    UFUNCTION(BlueprintImplementableEvent, Category = "MC2|Splash")
    void OnSplashComplete();

    /** WBP binds this to play the logo fade-in/out UWidgetAnimation. */
    UFUNCTION(BlueprintImplementableEvent, Category = "MC2|Splash")
    void OnStartLogoAnimation();

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry,
                                   const FKeyEvent& InKeyEvent) override;

private:
    FTimerHandle AutoSkipTimer;
    bool bSkipped = false;

    void DoComplete();
};
