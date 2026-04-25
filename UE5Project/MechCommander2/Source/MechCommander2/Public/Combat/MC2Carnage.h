#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MC2Carnage.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UAudioComponent;

UENUM(BlueprintType)
enum class EMC2CarnageType : uint8
{
	Explosion_Small   UMETA(DisplayName = "Explosion (Small)"),
	Explosion_Medium  UMETA(DisplayName = "Explosion (Medium)"),
	Explosion_Large   UMETA(DisplayName = "Explosion (Large)"),
	Fire              UMETA(DisplayName = "Fire"),
	Smoke             UMETA(DisplayName = "Smoke"),
	MechDestruction   UMETA(DisplayName = "Mech Destruction"),
	BuildingCollapse  UMETA(DisplayName = "Building Collapse"),
};

/**
 * AMC2Carnage
 * Wraps a Niagara particle effect + audio for transient combat effects.
 * Directly mirrors MC2's Carnage class (carnage.h) which handles
 * fires, explosions, and debris with auto-lifetime.
 *
 * Spawned by: AMC2Projectile on impact, AMC2Mover on destruction.
 * Auto-destroys after the Niagara system completes.
 */
UCLASS()
class MECHCOMMANDER2_API AMC2Carnage : public AActor
{
	GENERATED_BODY()

public:
	AMC2Carnage();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Carnage")
	TObjectPtr<UNiagaraComponent> NiagaraComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Carnage")
	TObjectPtr<UAudioComponent> AudioComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Carnage")
	EMC2CarnageType CarnageType = EMC2CarnageType::Explosion_Medium;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Carnage")
	float Duration = 3.f;     // seconds before auto-destroy; 0 = wait for Niagara system end

	// Radius for fire lingering effects — matches MC2's CarnageInfo::fire::radius
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Carnage")
	float Radius = 100.f;

	/** Spawn a carnage effect at a world location. Convenience factory. */
	UFUNCTION(BlueprintCallable, Category = "Carnage", meta = (WorldContext = "WorldContext"))
	static AMC2Carnage* SpawnCarnage(
		UObject* WorldContext,
		TSubclassOf<AMC2Carnage> CarnageClass,
		const FVector& Location,
		EMC2CarnageType Type
	);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnNiagaraFinished(UNiagaraComponent* PSystem);
};
