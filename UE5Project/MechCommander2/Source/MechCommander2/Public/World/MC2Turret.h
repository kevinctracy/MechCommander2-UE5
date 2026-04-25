#pragma once

#include "CoreMinimal.h"
#include "Units/MC2Mover.h"
#include "MC2Turret.generated.h"

/**
 * AMC2Turret
 * Extends AMC2Mover with a static body and rotating turret mesh.
 * Cannot translate; only the turret yaw mesh rotates to track targets.
 * Uses the same weapon/sensor/damage system as other movers.
 *
 * Blueprint child sets:
 *   - BaseMesh (static)
 *   - TurretMesh (yaws to aim)
 *   - WeaponHardpoints on TurretMesh socket
 *
 * Maps to MC2's Turret class (turret.h).
 */
UCLASS(BlueprintType)
class MECHCOMMANDER2_API AMC2Turret : public AMC2Mover
{
	GENERATED_BODY()

public:
	AMC2Turret();

	// --- Meshes ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret")
	TObjectPtr<UStaticMeshComponent> TurretMesh;

	// --- Turret rotation ---

	// Degrees per second the turret can rotate
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Turret")
	float TurretYawRate = 90.f;

	// Max yaw offset from forward (0 = no limit, 180 = full rotation)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Turret")
	float MaxTurretYaw = 0.f;

	// Current yaw offset of turret relative to base
	UPROPERTY(BlueprintReadOnly, Category = "Turret")
	float CurrentTurretYaw = 0.f;

	// --- Auto-attack ---

	// Turrets fire automatically at any detected enemy
	UPROPERTY(EditDefaultsOnly, Category = "Turret")
	bool bAutoFire = true;

	virtual void Tick(float DeltaSeconds) override;

	// Turrets can't move — override order functions to no-op
	virtual void ReceiveMoveOrder(const FVector& Destination) override {}
	virtual void ReceiveGuardOrder(const FVector& Position)   override {}

protected:
	virtual void BeginPlay() override;

private:
	void AimTurretAtTarget(float DeltaSeconds);
	void FireIfReady();

	TWeakObjectPtr<AMC2Mover> CurrentTarget;
};
