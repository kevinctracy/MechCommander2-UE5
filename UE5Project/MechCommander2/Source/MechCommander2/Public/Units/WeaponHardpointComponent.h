#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "WeaponHardpointComponent.generated.h"

class AMC2Projectile;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class EMC2WeaponType : uint8
{
	Energy      UMETA(DisplayName = "Energy"),       // PPC, ER Laser, Flamer
	Ballistic   UMETA(DisplayName = "Ballistic"),    // AC/2..20, MG, Gauss
	Missile     UMETA(DisplayName = "Missile"),      // LRM, SRM, Streak
	Artillery   UMETA(DisplayName = "Artillery"),    // off-map support
};

/**
 * UWeaponHardpointComponent
 * A single weapon mount on a mech or vehicle.
 * Matches MC2's COMPONENT_FORM_WEAPON system from cmponent.h.
 *
 * Handles: cooldown, ammo depletion, heat generation, fire animation notifies.
 */
UCLASS(ClassGroup=(MC2), meta=(BlueprintSpawnableComponent))
class MECHCOMMANDER2_API UWeaponHardpointComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UWeaponHardpointComponent();

	// --- Config (set from Data Table row at spawn) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 MasterComponentID = 0;    // compbas.csv MasterID

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FString WeaponName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EMC2WeaponType WeaponType = EMC2WeaponType::Energy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float Damage = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float Range = 600.f;            // effective range in UU

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float HeatGenerated = 3.f;      // heat per shot

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float Cooldown = 1.5f;          // seconds between shots

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 MaxAmmo = -1;             // -1 = unlimited (energy weapons)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 CurrentAmmo = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 NumHeatsinks = 0;         // heatsinks installed in this slot

	// --- Projectile / Visual ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Effects")
	TSubclassOf<AMC2Projectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Effects")
	TObjectPtr<UNiagaraSystem> MuzzleFlashEffect;

	// --- State ---

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	float CooldownRemaining = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool bDestroyed = false;

	// --- API ---

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool CanFire() const;

	/** Fire at a world-space target. Returns true if a shot was fired. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool FireAt(const FVector& TargetLocation, AActor* TargetActor);

	/** Heat dissipation bonus this hardpoint contributes (from heatsinks). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon")
	float GetHeatSinkBonus() const { return NumHeatsinks * 1.5f; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void DestroyWeapon();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void SpawnProjectile(const FVector& TargetLocation, AActor* TargetActor);
	void SpawnHitscan(const FVector& TargetLocation, AActor* TargetActor);
};
