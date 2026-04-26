#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MC2FXLibrary.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

/**
 * EMC2FXType
 * Maps to GosFX effect definitions in mc2.fx.
 * Each entry has a corresponding NS_MC2_* Niagara asset (created in editor).
 *
 * GosFX → Niagara asset name mapping:
 *   FX_ExplosionSmall   → NS_MC2_Explosion_Small   (radius ~3m, orange/white, 0.5s)
 *   FX_ExplosionMedium  → NS_MC2_Explosion_Medium  (radius ~8m, 0.8s, smoke lingers)
 *   FX_ExplosionLarge   → NS_MC2_Explosion_Large   (radius ~15m, 1.2s, shockwave ring)
 *   FX_MuzzleEnergy     → NS_MC2_Muzzle_Energy     (blue-white flash, 0.1s)
 *   FX_MuzzleBallistic  → NS_MC2_Muzzle_Ballistic  (yellow flash + smoke puff, 0.15s)
 *   FX_MuzzleMissile    → NS_MC2_Muzzle_Missile    (white smoke trail emitter, looping while missile flies)
 *   FX_SmokeTrail       → NS_MC2_Smoke_Trail       (grey ribbon, looping — damaged mech)
 *   FX_Fire             → NS_MC2_Fire              (orange/yellow, looping — burning wreck)
 *   FX_DustCloud        → NS_MC2_Dust_Cloud        (tan/brown puff, 1s — footstep/impact)
 *   FX_HitSpark         → NS_MC2_Hit_Spark         (white/yellow sparks, 0.2s — projectile impact)
 *   FX_WaterSplash      → NS_MC2_Water_Splash      (blue spray, 0.4s)
 *   FX_JumpJetExhaust   → NS_MC2_JumpJet           (blue-white cone exhaust, looping while jumping)
 */
UENUM(BlueprintType)
enum class EMC2FXType : uint8
{
	ExplosionSmall    UMETA(DisplayName = "Explosion Small"),
	ExplosionMedium   UMETA(DisplayName = "Explosion Medium"),
	ExplosionLarge    UMETA(DisplayName = "Explosion Large"),
	MuzzleEnergy      UMETA(DisplayName = "Muzzle Flash (Energy)"),
	MuzzleBallistic   UMETA(DisplayName = "Muzzle Flash (Ballistic)"),
	MuzzleMissile     UMETA(DisplayName = "Muzzle Flash (Missile)"),
	SmokeTrail        UMETA(DisplayName = "Smoke Trail"),
	Fire              UMETA(DisplayName = "Fire"),
	DustCloud         UMETA(DisplayName = "Dust Cloud"),
	HitSpark          UMETA(DisplayName = "Hit Spark"),
	WaterSplash       UMETA(DisplayName = "Water Splash"),
	JumpJetExhaust    UMETA(DisplayName = "Jump Jet Exhaust"),
};

/**
 * UMC2FXLibrary
 * Blueprint-callable helpers for spawning MC2 Niagara effects by type enum.
 * Niagara assets are loaded from /Game/FX/ by name convention NS_MC2_*.
 * All spawn calls are no-ops if the asset isn't found (safe during early dev).
 */
UCLASS()
class MECHCOMMANDER2_API UMC2FXLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Spawn a one-shot effect at a world location (fire-and-forget).
	UFUNCTION(BlueprintCallable, Category = "MC2|FX", meta = (WorldContext = "WorldContext"))
	static UNiagaraComponent* SpawnFX(
		const UObject* WorldContext,
		EMC2FXType FXType,
		FVector Location,
		FRotator Rotation = FRotator::ZeroRotator,
		float Scale = 1.f
	);

	// Spawn a looping effect attached to an actor component (e.g. smoke trail on a mech).
	// Caller is responsible for deactivating when no longer needed.
	UFUNCTION(BlueprintCallable, Category = "MC2|FX")
	static UNiagaraComponent* SpawnFXAttached(
		EMC2FXType FXType,
		USceneComponent* AttachTo,
		FName SocketName = NAME_None,
		FVector LocationOffset = FVector::ZeroVector,
		float Scale = 1.f
	);

	// Get the soft path to a Niagara asset by type (for async loading).
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MC2|FX")
	static FSoftObjectPath GetFXAssetPath(EMC2FXType FXType);

	// Returns the recommended scale for an explosion based on BattleRating (1-20).
	// Matches GosFX effect magnitude to MC2's explosion size per unit type.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MC2|FX")
	static float GetExplosionScaleForBR(int32 BattleRating);

private:
	// Returns asset path string for a given FX type.
	// Pattern: /Game/FX/NS_MC2_{TypeName}.NS_MC2_{TypeName}
	static FString GetAssetPathString(EMC2FXType FXType);

	// Cached Niagara system refs (loaded on first use)
	static TMap<EMC2FXType, TWeakObjectPtr<UNiagaraSystem>> FXCache;
	static UNiagaraSystem* GetOrLoadFX(EMC2FXType FXType);
};
