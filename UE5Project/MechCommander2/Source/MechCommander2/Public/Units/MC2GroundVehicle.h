#pragma once

#include "CoreMinimal.h"
#include "Units/MC2Mover.h"
#include "MC2GroundVehicle.generated.h"

/**
 * EMC2VehicleArc
 * 5-arc vehicle hit model from gvehicl.h (NUM_GROUNDVEHICLE_HIT_ARCS = 5).
 * Vehicles are simpler than mechs: no per-section crits, just arc depletion.
 */
UENUM(BlueprintType)
enum class EMC2VehicleArc : uint8
{
	Front  UMETA(DisplayName = "Front"),
	Rear   UMETA(DisplayName = "Rear"),
	Left   UMETA(DisplayName = "Left"),
	Right  UMETA(DisplayName = "Right"),
	Turret UMETA(DisplayName = "Turret"),
};

USTRUCT(BlueprintType)
struct FMC2VehicleArmor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMC2VehicleArc Arc = EMC2VehicleArc::Front;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxArmor = 20.f;

	UPROPERTY(BlueprintReadOnly)
	float CurrentArmor = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxInternalStructure = 10.f;

	UPROPERTY(BlueprintReadOnly)
	float CurrentInternalStructure = 10.f;
};

/**
 * AMC2GroundVehicle
 * C++ base for wheeled/tracked vehicles (tanks, APCs, artillery).
 * Extends AMC2Mover with 5-arc armor, turret yaw rotation, and simpler
 * destruction model (no BT crit system — vehicle is destroyed when any arc
 * reaches 0 internal structure).
 *
 * Blueprint subclass: BP_GroundVehicle
 * Vehicle types defined in DT_VehicleTypes (FMC2VehicleTypeRow).
 */
UCLASS(Abstract)
class MECHCOMMANDER2_API AMC2GroundVehicle : public AMC2Mover
{
	GENERATED_BODY()

public:
	AMC2GroundVehicle();

	// --- 5-Arc armor (Front/Rear/Left/Right/Turret) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_VehicleArmor, Category = "Combat|Armor")
	TArray<FMC2VehicleArmor> VehicleArmor;

	UFUNCTION()
	void OnRep_VehicleArmor();

	// Apply damage to a specific arc. Returns true if the vehicle was destroyed.
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool ApplyVehicleDamage(float Damage, EMC2VehicleArc HitArc);

	// Get current armor on an arc
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
	float GetArcArmor(EMC2VehicleArc Arc) const;

	// --- Turret ---

	// Maximum yaw the turret can rotate from vehicle forward (degrees, 0-180)
	// 360 = full rotation (e.g. tanks with rotating turrets)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Turret")
	float MaxTurretYaw = 360.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Turret")
	float TurretYawRate = 90.f;   // degrees per second

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Turret")
	float CurrentTurretYaw = 0.f;  // relative to vehicle forward

	// Aim turret toward a world-space point (called each frame by AI/player input)
	UFUNCTION(BlueprintCallable, Category = "Turret")
	void AimTurretAt(const FVector& WorldTarget);

	// Bone name for the turret mesh — this is rotated separately from the hull
	UPROPERTY(EditDefaultsOnly, Category = "Turret")
	FName TurretBone = TEXT("turret");

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	// Arc that incoming damage hits based on relative angle to attacker
	EMC2VehicleArc GetHitArcFromDirection(const FVector& DamageOrigin) const;

private:
	FRotator TargetTurretRotation = FRotator::ZeroRotator;

	void InitDefaultArmor();
};
