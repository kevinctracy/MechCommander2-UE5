#pragma once

#include "CoreMinimal.h"
#include "Units/MC2Mover.h"
#include "MC2BattleMech.generated.h"

/**
 * AMC2BattleMech
 * C++ base for all BattleMech actors (BP_BattleMech extends this).
 * Key differentiator from AMC2Mover: the torso can aim independently of the
 * leg direction, matching MC2's body section twist system.
 *
 * Torso twist data flow:
 *   1. AI/PlayerController calls SetTorsoAimDirection(WorldDir) or AimTorsoAt(WorldLoc)
 *   2. This updates TorsoYawOffset (clamped to ±MaxTorsoYaw, from DT_MechChassis)
 *   3. ABP_BattleMech reads TorsoYawOffset each frame and blends the torso bone
 *      via a "Modify Bone" node in the Animation Blueprint
 *
 * Jump Jets: if bHasJumpJets = true, CanJump() allows the AI to order a jump.
 * Jump arc is handled in ABP_BattleMech via a "Fall/GetUp" state.
 */
UCLASS(BlueprintType)
class MECHCOMMANDER2_API AMC2BattleMech : public AMC2Mover
{
	GENERATED_BODY()

public:
	AMC2BattleMech();

	// --- Torso twist (P2.2.3) ---

	// Current torso yaw offset from leg direction, degrees. Negative = left.
	// Read by ABP_BattleMech to drive the "Modify Bone" torso node.
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "BattleMech|Torso")
	float TorsoYawOffset = 0.f;

	// Maximum torso yaw in either direction. Set from DT_MechChassis::TorsoYawRate at spawn.
	// Default 120° matches MC2's standard mech torso limit.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleMech|Torso")
	float MaxTorsoYaw = 120.f;

	// Degrees per second the torso rotates toward the aim direction.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleMech|Torso")
	float TorsoYawRate = 90.f;

	// Set the aim direction as a world-space unit vector (from AI or hitscan targeting).
	// Updates DesiredTorsoYaw; actual rotation interpolated in Tick.
	UFUNCTION(BlueprintCallable, Category = "BattleMech|Torso")
	void SetTorsoAimDirection(const FVector& WorldDirection);

	// Convenience: compute aim direction from a world location.
	UFUNCTION(BlueprintCallable, Category = "BattleMech|Torso")
	void AimTorsoAt(const FVector& WorldLocation);

	// --- Jump Jets (P2.2.2 jump state) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BattleMech|JumpJets")
	bool bHasJumpJets = false;

	// Maximum horizontal jump range in cm (0 = no jump capability).
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BattleMech|JumpJets")
	float JumpRange = 0.f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "BattleMech|JumpJets")
	bool bIsJumping = false;

	UFUNCTION(BlueprintCallable, Category = "BattleMech|JumpJets")
	void StartJump(const FVector& TargetLocation);

	UFUNCTION(BlueprintCallable, Category = "BattleMech|JumpJets")
	void LandFromJump();

	// --- Chassis data link ---

	// Set at spawn from DT_MechChassis row to configure torso limits, jump range etc.
	UFUNCTION(BlueprintCallable, Category = "BattleMech")
	void InitFromChassisID(FName ChassisID, UDataTable* ChassisTable);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

private:
	float DesiredTorsoYaw = 0.f;

	// Jump state
	FVector JumpOrigin;
	FVector JumpTarget;
	float   JumpAlpha    = 0.f;
	float   JumpDuration = 1.2f;   // seconds to complete the arc
};
