#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MC2PilotComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPilotKilled, const FName&, PilotID);

/**
 * UMC2PilotComponent
 * Tracks the pilot assigned to a mover — skill modifiers, injury, and KIA status.
 * Attached to AMC2Mover at BeginPlay; removed/replaced when a pilot is reassigned.
 *
 * Skill system (BattleTech standard, lower = better):
 *   Gunnery   — rolls against this to hit a target (base hit chance = 100% - Gunnery×8%)
 *   Piloting  — rolls against this for fall-avoidance, gyro check etc.
 *
 * Maps to MC2's MechWarrior class (warrior.h).
 */
UCLASS(BlueprintType, ClassGroup=(MC2), meta=(BlueprintSpawnableComponent))
class MECHCOMMANDER2_API UMC2PilotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMC2PilotComponent();

	// --- Pilot identity ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pilot")
	FName PilotID;   // DT_Pilots row key; empty = generic AI pilot

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pilot")
	FString PilotName = TEXT("Unknown Pilot");

	// --- Skills (3 = veteran, 4 = regular, 5 = green, matching BT conventions) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pilot|Skills")
	int32 Gunnery  = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pilot|Skills")
	int32 Piloting = 5;

	// --- Injury tracking ---

	UPROPERTY(BlueprintReadOnly, Category = "Pilot|Injury")
	int32 InjuryLevel = 0;  // 0=fine, 1=light, 2=moderate, 3=severe, 4=KIA

	UPROPERTY(BlueprintReadOnly, Category = "Pilot|Injury")
	bool bIsKIA = false;

	// --- XP (accumulated in-mission; committed to save on mission end) ---

	UPROPERTY(BlueprintReadOnly, Category = "Pilot|XP")
	int32 MissionXP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Pilot|XP")
	int32 MissionKills = 0;

	// --- Events ---

	UPROPERTY(BlueprintAssignable, Category = "Pilot")
	FOnPilotKilled OnPilotKilled;

	// --- API ---

	// Returns gunnery skill with injury penalty applied (each injury level +1 effective gunnery)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pilot")
	int32 GetEffectiveGunnery() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pilot")
	int32 GetEffectivePiloting() const;

	// Base hit probability: Gunnery 4 = 58% base. Range/LOS modifiers applied elsewhere.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pilot")
	float GetBaseHitChance() const;

	// Apply a pilot injury hit (from cockpit/head damage, mech fall, etc.)
	UFUNCTION(BlueprintCallable, Category = "Pilot")
	void TakeInjury(int32 Severity = 1);

	// Record a kill for XP purposes
	UFUNCTION(BlueprintCallable, Category = "Pilot")
	void RecordKill();

	// Roll a piloting check (for falls); returns true if passed
	UFUNCTION(BlueprintCallable, Category = "Pilot")
	bool RollPilotingCheck(int32 Modifier = 0);

	// Commit mission XP to the logistics subsystem save game
	UFUNCTION(BlueprintCallable, Category = "Pilot")
	void CommitXPToSave();

protected:
	virtual void BeginPlay() override;
};
