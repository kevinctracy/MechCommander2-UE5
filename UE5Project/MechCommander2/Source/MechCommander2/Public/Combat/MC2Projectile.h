#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MC2Projectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UNiagaraSystem;

/**
 * AMC2Projectile
 * Base class for all physical projectiles: ballistic shells, SRMs, LRMs.
 * Energy weapons use hitscan (WeaponHardpointComponent::SpawnHitscan) instead.
 *
 * Subclass in Blueprint:
 *   BP_Projectile_AC5    — medium autocannon shell, high velocity, no homing
 *   BP_Projectile_LRM    — long range missile, homing enabled, slow
 *   BP_Projectile_SRM    — short range missile, fast, no homing
 *   BP_Projectile_Gauss  — very high velocity, heavy mass, kinetic damage
 */
UCLASS()
class MECHCOMMANDER2_API AMC2Projectile : public AActor
{
	GENERATED_BODY()

public:
	AMC2Projectile();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// --- Config ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
	float Damage = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
	float SplashRadius = 0.f;       // > 0 = area damage (artillery, LRM cluster)

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
	float Lifetime = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
	bool bIsHoming = false;

	// Optional override — if unset, uses UMC2FXLibrary::HitSpark + crater decal automatically
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Effects")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Effects")
	TObjectPtr<UNiagaraSystem> TrailEffect;

	// Decal material for bullet/impact craters on terrain and buildings.
	// Assign M_BulletCrater (BC1 terrain) or M_MissileCrater in Blueprint subclasses.
	// If null, no decal is spawned (energy weapons should leave no decal).
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Effects")
	TObjectPtr<class UMaterialInterface> CraterDecalMaterial;

	// Size of the decal projected onto terrain (uniform XYZ, cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Effects")
	float CraterDecalSize = 80.f;

	// How long the crater decal fades out (seconds; 30s default matches MC2 crater persistence)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Effects")
	float CraterDecalLifetime = 30.f;

	// --- Runtime ---

	UPROPERTY(BlueprintReadWrite, Category = "Projectile")
	TWeakObjectPtr<AActor> TargetActor;

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void Launch(const FVector& TargetLocation);

	// --- Anti-Missile System interception (called by AMS on nearby units) ---

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void Intercept();   // destroys this projectile without impact

	UFUNCTION(BlueprintImplementableEvent, Category = "Projectile")
	void BP_OnImpact(const FHitResult& Hit);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	           UPrimitiveComponent* OtherComp, FVector NormalImpulse,
	           const FHitResult& Hit);

private:
	void ApplyImpactDamage(const FHitResult& Hit);
	void ApplySplashDamage(const FVector& Location);
};


// ---------------------------------------------------------------------------
// AMC2Missile — homing variant
// ---------------------------------------------------------------------------

UCLASS()
class MECHCOMMANDER2_API AMC2Missile : public AMC2Projectile
{
	GENERATED_BODY()

public:
	AMC2Missile();

	// How aggressively the missile tracks its target
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Missile")
	float HomingAcceleration = 5000.f;

	// Arm time before homing activates (prevents self-intercept)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Missile")
	float ArmTime = 0.3f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	float TimeAlive = 0.f;
};
