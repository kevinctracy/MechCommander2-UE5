#include "UI/MC2DialogWidget.h"
#include "GameFramework/PlayerController.h"

void UMC2DialogWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(true);
}

UMC2DialogWidget* UMC2DialogWidget::ShowDialog(APlayerController* PC,
                                               const FText& Title, const FText& Body,
                                               const FText& ConfirmLabel,
                                               const FText& CancelLabel,
                                               EMC2DialogMode Mode)
{
    if (!PC) return nullptr;

    UMC2DialogWidget* Widget = CreateWidget<UMC2DialogWidget>(PC, StaticClass());
    if (!Widget) return nullptr;

    Widget->SetMode(Mode);
    Widget->SetDialogContent(Title, Body, ConfirmLabel, CancelLabel);
    Widget->AddToViewport(100);  // high Z-order so dialog appears above everything
    return Widget;
}

void UMC2DialogWidget::SetDialogContent(const FText& InTitle, const FText& InBody,
                                        const FText& InConfirmLabel,
                                        const FText& InCancelLabel)
{
    OnDialogContentSet(InTitle, InBody, InConfirmLabel, InCancelLabel);
}

void UMC2DialogWidget::SetMode(EMC2DialogMode InMode)
{
    Mode = InMode;
    OnModeChanged(Mode);
}

FString UMC2DialogWidget::GetInputText() const
{
    return CachedInputText.ToString();
}

void UMC2DialogWidget::Dismiss(bool bConfirmed)
{
    OnResult.Broadcast(bConfirmed);
    // WBP plays LeaveAnim (Y → -466 in 0.1s) and calls RemoveFromParent on anim end
    RemoveFromParent();
}
