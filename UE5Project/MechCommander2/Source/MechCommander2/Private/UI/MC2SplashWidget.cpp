#include "UI/MC2SplashWidget.h"
#include "TimerManager.h"
#include "Engine/World.h"

void UMC2SplashWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(true);
}

void UMC2SplashWidget::StartSplash()
{
    bSkipped = false;
    OnStartLogoAnimation();

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(AutoSkipTimer,
            FTimerDelegate::CreateUObject(this, &UMC2SplashWidget::DoComplete),
            SplashDuration, false);
    }
}

void UMC2SplashWidget::HandleSkip()
{
    if (!bSkipped)
        DoComplete();
}

void UMC2SplashWidget::DoComplete()
{
    if (bSkipped) return;
    bSkipped = true;

    if (UWorld* World = GetWorld())
        World->GetTimerManager().ClearTimer(AutoSkipTimer);

    OnSplashComplete();
    RemoveFromParent();
}

FReply UMC2SplashWidget::NativeOnKeyDown(const FGeometry& InGeometry,
                                          const FKeyEvent& InKeyEvent)
{
    HandleSkip();
    return FReply::Handled();
}
