#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MC2Building.generated.h"

class UMC2HealthComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class UAudioComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuildingDestroyed, AMC2Building*, Building);

/**
 * AMC2Building
 * Destructible, immobile structure.
 * Has armor/IS via UMC2HealthComponent but no movement.
 * On destruction: swaps to destroyed mesh + plays explosion Carnage.
 *
 * Maps to MC2's Building GameObject hierarchy.
 * Place in level; configure DestroyedMesh and CarnageClass in Blueprint defaults.
 */
UCLASS(BlueprintType)
class MECHCOMMANDER2_API AMC2Building : public AActor
{
	GENERATED_BODY()

public:
	AMC2Building();

	// --- Mesh ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// Swapped in after destruction
	UPROPERTY(EditDefaultsOnly, Category = "Building")
	TObjectPtr<UStaticMesh> DestroyedMesh;

	// --- Health ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building")
	TObjectPtr<UMC2HealthComponent> HealthComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Building")
	float MaxHitPoints = 500.f;

	// --- Team (buildings are owned by a team; enemy buildings are valid targets) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	int32 TeamIndex = 1;   // 1 = enemy by default

	// --- Destruction ---

	UPROPERTY(EditDefaultsOnly, Category = "Building")
	TSubclassOf<class AMC2Carnage> ExplosionCarnageClass;

	UPROPERTY(EditDefaultsOnly, Category = "Building")
	float ExplosionRadius = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Building")
	float ExplosionDamage = 100.f;

	UPROPERTY(BlueprintReadOnly, Category = "Building")
	bool bIsDestroyed = false;

	UPROPERTY(BlueprintAssignable, Category = "Building")
	FOnBuildingDestroyed OnBuildingDestroyed;

	// Called by UMC2HealthComponent when HP reaches 0
	UFUNCTION()
	void OnHealthDepleted(AActor* Instigator);

	// Explicitly destroy (e.g. from mission script)
	UFUNCTION(BlueprintCallable, Category = "Building")
	void DestroyBuilding(AActor* Instigator = nullptr);

protected:
	virtual void BeginPlay() override;
};
