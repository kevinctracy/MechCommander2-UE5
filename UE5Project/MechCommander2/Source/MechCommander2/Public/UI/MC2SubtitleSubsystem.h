#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MC2SubtitleSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSubtitleChanged,
	const FText&, SubtitleText,
	float,        Duration);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSubtitleCleared);

/**
 * FMC2SubtitleEntry
 * One item in the subtitle queue.
 */
USTRUCT(BlueprintType)
struct FMC2SubtitleEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FText    Text;

	UPROPERTY(BlueprintReadOnly)
	FText    Speaker;   // e.g. "BETTY", "COMMANDER HAYES"

	UPROPERTY(BlueprintReadOnly)
	float    Duration   = 3.f;  // seconds on screen

	UPROPERTY(BlueprintReadOnly)
	FLinearColor Color  = FLinearColor::White;
};

/**
 * UMC2SubtitleSubsystem
 * Queues and ticks subtitle lines for VO and cinematic dialogue.
 *
 * Maps to MC2's original bettyManager / cinematic subtitle display.
 * WBP_Subtitles binds to OnSubtitleChanged / OnSubtitleCleared.
 *
 * Usage:
 *   Subsystem->EnqueueSubtitle({Text, Speaker, Duration, Color});
 *   Subsystem->ClearSubtitles();
 */
UCLASS()
class MECHCOMMANDER2_API UMC2SubtitleSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Fired when a new subtitle becomes active
	UPROPERTY(BlueprintAssignable, Category = "Subtitles")
	FOnSubtitleChanged OnSubtitleChanged;

	// Fired when the last queued subtitle expires
	UPROPERTY(BlueprintAssignable, Category = "Subtitles")
	FOnSubtitleCleared OnSubtitleCleared;

	// Add a subtitle to the back of the queue
	UFUNCTION(BlueprintCallable, Category = "Subtitles")
	void EnqueueSubtitle(const FMC2SubtitleEntry& Entry);

	// Convenience: enqueue a plain text line with a given duration
	UFUNCTION(BlueprintCallable, Category = "Subtitles")
	void ShowSubtitle(const FText& Text, float Duration = 3.f);

	// Discard all queued and active subtitles immediately
	UFUNCTION(BlueprintCallable, Category = "Subtitles")
	void ClearSubtitles();

	// Returns the currently-displayed entry (invalid Text if nothing showing)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Subtitles")
	FMC2SubtitleEntry GetCurrentSubtitle() const { return CurrentEntry; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Subtitles")
	bool IsShowingSubtitle() const { return bActive; }

	// UGameInstanceSubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	TArray<FMC2SubtitleEntry> Queue;
	FMC2SubtitleEntry         CurrentEntry;
	bool                      bActive = false;
	float                     RemainingTime = 0.f;

	FTimerHandle TickTimer;

	void AdvanceQueue();
	void OnTick();
};
