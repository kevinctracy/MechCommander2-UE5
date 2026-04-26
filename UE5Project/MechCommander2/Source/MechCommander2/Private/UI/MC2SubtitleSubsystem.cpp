#include "UI/MC2SubtitleSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"

static constexpr float TICK_RATE = 0.1f; // check expiry 10×/sec

void UMC2SubtitleSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (UWorld* World = GetGameInstance()->GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TickTimer, this, &UMC2SubtitleSubsystem::OnTick, TICK_RATE, true);
	}
}

void UMC2SubtitleSubsystem::Deinitialize()
{
	if (UWorld* World = GetGameInstance()->GetWorld())
	{
		World->GetTimerManager().ClearTimer(TickTimer);
	}
	Super::Deinitialize();
}

void UMC2SubtitleSubsystem::EnqueueSubtitle(const FMC2SubtitleEntry& Entry)
{
	Queue.Add(Entry);
	if (!bActive)
	{
		AdvanceQueue();
	}
}

void UMC2SubtitleSubsystem::ShowSubtitle(const FText& Text, float Duration)
{
	FMC2SubtitleEntry E;
	E.Text     = Text;
	E.Duration = Duration;
	EnqueueSubtitle(E);
}

void UMC2SubtitleSubsystem::ClearSubtitles()
{
	Queue.Empty();
	CurrentEntry  = FMC2SubtitleEntry{};
	bActive       = false;
	RemainingTime = 0.f;
	OnSubtitleCleared.Broadcast();
}

void UMC2SubtitleSubsystem::AdvanceQueue()
{
	if (Queue.IsEmpty())
	{
		CurrentEntry  = FMC2SubtitleEntry{};
		bActive       = false;
		OnSubtitleCleared.Broadcast();
		return;
	}

	CurrentEntry  = Queue[0];
	Queue.RemoveAt(0);
	bActive       = true;
	RemainingTime = CurrentEntry.Duration;

	OnSubtitleChanged.Broadcast(CurrentEntry.Text, CurrentEntry.Duration);
}

void UMC2SubtitleSubsystem::OnTick()
{
	if (!bActive) { return; }

	RemainingTime -= TICK_RATE;
	if (RemainingTime <= 0.f)
	{
		AdvanceQueue();
	}
}
