#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Units/MC2Mover.h"
#include "MC2HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnArmorDamaged,
	AMC2Mover*, Mover,
	FName, BoneName,
	float, DamageApplied,
	float, ArmorRemaining
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSectionDestroyed, AMC2Mover*, Mover, FName, BoneName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoverKilled, AMC2Mover*, Mover);

/**
 * UMC2HealthComponent
 * Location-based damage routing matching MC2's armor zone system from mover.h.
 *
 * Damage flow:
 *   1. Hit bone name -> nearest armor zone lookup
 *   2. Reduce armor; overflow to internal structure
 *   3. On IS destruction: trigger component critical hit roll
 *   4. On all armor+IS gone: signal mover destroyed
 *
 * Mech-specific: 12 zones (4 arcs × 3 sections)
 * Vehicle-specific: 5 zones (Front/Rear/Left/Right/Turret)
 * Building: single zone
 */
UCLASS(ClassGroup=(MC2), meta=(BlueprintSpawnableComponent))
class MECHCOMMANDER2_API UMC2HealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMC2HealthComponent();

	// --- Events ---

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnArmorDamaged OnArmorDamaged;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnSectionDestroyed OnSectionDestroyed;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnMoverKilled OnMoverKilled;

	// --- Critical hit config ---

	// Probability [0-1] of a critical hit when IS is depleted in a zone
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Crits")
	float CriticalHitChance = 0.25f;

	// --- API ---

	/**
	 * Apply damage to the unit, routed through the correct armor zone.
	 * @param Damage      Raw damage points
	 * @param HitBone     Bone name from the skeletal mesh hit result
	 * @param Instigator  Actor that caused the damage
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ApplyDamage(float Damage, FName HitBone, AActor* Instigator);

	/**
	 * Direct damage to a specific arc/section (used by splash / area damage).
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ApplyDamageToZone(float Damage, EMC2HitArc Arc, EMC2HitSection Section, AActor* Instigator);

	/** Returns total remaining armor across all zones (for HUD). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
	float GetTotalArmorPercent() const;

	/** True if this unit has been destroyed (all IS gone). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
	bool IsDestroyed() const { return bDestroyed; }

	// --- Critical hit consequence tracking ---

	// Number of engine crits sustained (2+ = destroyed)
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Crits")
	int32 EngineCritCount = 0;

	// Number of gyro crits (1 = piloting penalty, 2 = destabilized)
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Crits")
	int32 GyroCritCount = 0;

	// Blueprint events for each crit consequence type
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Crits")
	void OnEngineCrit(int32 NewCritCount);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Crits")
	void OnGyroCrit(int32 NewCritCount);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Crits")
	void OnWeaponCrit(int32 HardpointIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Crits")
	void OnActuatorCrit(FName BoneName);

protected:
	virtual void BeginPlay() override;

private:
	bool bDestroyed = false;

	// Find the best armor zone for a given bone name
	FMC2ArmorZone* FindZoneForBone(FName BoneName);

	// Fallback: determine arc from hit bone name heuristics
	EMC2HitArc ArcFromBoneName(FName BoneName) const;
	EMC2HitSection SectionFromBoneName(FName BoneName) const;

	void ApplyDamageToZoneData(FMC2ArmorZone& Zone, float Damage, AActor* Instigator);
	void TriggerCriticalHit(const FMC2ArmorZone& Zone);
	void ApplyCritConsequences(const FMC2ArmorZone& Zone);
	void CheckDestroyed();
};
