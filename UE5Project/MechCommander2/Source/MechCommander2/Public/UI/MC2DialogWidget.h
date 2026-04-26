#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MC2DialogWidget.generated.h"

/**
 * Reusable modal dialog widget.
 * Covers all three MC2 dialog variants (mcl_dialog.fit / mcl_dialog_onebutton.fit / mcl_dialog_edit.fit).
 *
 * Usage:
 *   auto* D = UMC2DialogWidget::ShowDialog(PC, Title, Body,
 *       FText::FromString("OK"), FText::FromString("Cancel"),
 *       FOnMC2DialogResult::CreateUObject(this, &AMyActor::OnDialogResult));
 *
 * The FIT describes a slide-in animation (Y: -466 → 0 in 0.15s) and slide-out on dismiss.
 * Replicate with a UWidgetAnimation bound to OnAppear/OnDismiss.
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMC2DialogResult, bool, bConfirmed);

UENUM(BlueprintType)
enum class EMC2DialogMode : uint8
{
    OkCancel   UMETA(DisplayName = "OK + Cancel"),   // mcl_dialog.fit
    OkOnly     UMETA(DisplayName = "OK Only"),        // mcl_dialog_onebutton.fit
    OkCancelEdit UMETA(DisplayName = "OK + Cancel + Text Input"), // mcl_dialog_edit.fit
};

UCLASS(Abstract)
class MECHCOMMANDER2_API UMC2DialogWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // ── Statics ───────────────────────────────────────────────────────────────

    /** Create, configure, and add the dialog to the viewport. Returns the widget. */
    UFUNCTION(BlueprintCallable, Category = "MC2|Dialog",
              meta = (DefaultToSelf = "WorldContext", WorldContext = "WorldContext"))
    static UMC2DialogWidget* ShowDialog(APlayerController* PC,
                                        const FText& Title, const FText& Body,
                                        const FText& ConfirmLabel,
                                        const FText& CancelLabel,
                                        EMC2DialogMode Mode = EMC2DialogMode::OkCancel);

    // ── API ───────────────────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "MC2|Dialog")
    void SetDialogContent(const FText& InTitle, const FText& InBody,
                          const FText& InConfirmLabel, const FText& InCancelLabel);

    UFUNCTION(BlueprintCallable, Category = "MC2|Dialog")
    void SetMode(EMC2DialogMode InMode);

    /** Text entered in OkCancelEdit mode. Empty for other modes. */
    UFUNCTION(BlueprintCallable, Category = "MC2|Dialog")
    FString GetInputText() const;

    /** Play dismiss animation then RemoveFromParent. */
    UFUNCTION(BlueprintCallable, Category = "MC2|Dialog")
    void Dismiss(bool bConfirmed);

    // ── Delegates ─────────────────────────────────────────────────────────────

    UPROPERTY(BlueprintAssignable, Category = "MC2|Dialog")
    FOnMC2DialogResult OnResult;

    // ── Blueprint hooks ───────────────────────────────────────────────────────

    UFUNCTION(BlueprintImplementableEvent, Category = "MC2|Dialog")
    void OnDialogContentSet(const FText& Title, const FText& Body,
                            const FText& ConfirmLabel, const FText& CancelLabel);

    UFUNCTION(BlueprintImplementableEvent, Category = "MC2|Dialog")
    void OnModeChanged(EMC2DialogMode NewMode);

    /** Called by "OK" button binding in WBP. */
    UFUNCTION(BlueprintCallable, Category = "MC2|Dialog")
    void HandleConfirm() { Dismiss(true); }

    /** Called by "Cancel" button binding in WBP. */
    UFUNCTION(BlueprintCallable, Category = "MC2|Dialog")
    void HandleCancel() { Dismiss(false); }

protected:
    UPROPERTY(BlueprintReadOnly, Category = "MC2|Dialog")
    EMC2DialogMode Mode = EMC2DialogMode::OkCancel;

    UPROPERTY(BlueprintReadOnly, Category = "MC2|Dialog")
    FText CachedInputText;

    virtual void NativeConstruct() override;
};
