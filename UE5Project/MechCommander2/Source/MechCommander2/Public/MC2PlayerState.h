#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MC2PlayerState.generated.h"

/**
 * AMC2PlayerState
 * Replicated per-player data: team, commander name, ready status for MP lobby.
 * Maps to MC2's player slot in the multiplayer session (MCL_MP player list).
 */
UCLASS()
class MECHCOMMANDER2_API AMC2PlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	// Team assignment (set by host)
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Player")
	int32 TeamIndex = 0;

	// Whether this player has confirmed their lance in the lobby
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Player")
	bool bIsReady = false;

	// Commander / callsign name shown in lobby
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Player")
	FString CommanderName = TEXT("Commander");

	// Current mission kill count (shown in debrief)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player|Stats")
	int32 MissionKills = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player|Stats")
	int32 MissionLosses = 0;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Called by AI controller when a kill is credited to this player
	UFUNCTION(BlueprintCallable, Category = "Player|Stats")
	void AddKill() { MissionKills++; }

	UFUNCTION(BlueprintCallable, Category = "Player|Stats")
	void AddLoss() { MissionLosses++; }
};
