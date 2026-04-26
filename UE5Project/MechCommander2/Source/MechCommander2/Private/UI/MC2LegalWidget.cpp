#include "UI/MC2LegalWidget.h"

void UMC2LegalWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(true);
    // WBP plays the enter slide animation (Y: -600→0, 0.23s) via "Play On Construct"
}

void UMC2LegalWidget::SetLegalText(const FText& HeaderText,
                                    const FText& BodyText,
                                    const FText& AcceptLabel)
{
    OnLegalDataSet(HeaderText, BodyText, AcceptLabel);
}

void UMC2LegalWidget::HandleAccept()
{
    if (bAccepted) return;
    bAccepted = true;

    OnLegalAccepted.Broadcast();
    OnPlayLeaveAnim();
    // DoRemove() called by WBP when leave animation finishes
}

void UMC2LegalWidget::DoRemove()
{
    RemoveFromParent();
}
