#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SensorComponent.generated.h"

class AMC2Mover;

USTRUCT(BlueprintType)
struct FMC2ContactInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<AMC2Mover> Unit;

	UPROPERTY(BlueprintReadOnly)
	float DistanceSq = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bHasLineOfSight = false;

	UPROPERTY(BlueprintReadOnly)
	float LastSeenTime = 0.f;
};

/**
 * USensorComponent
 * Implements MC2's SensorSystem: detects enemy units within sensor range,
 * performs line-of-sight checks, and maintains a contact list.
 * ECM jamming reduces effective sensor range against the ECM unit.
 *
 * Updates every SensorTickInterval seconds (default 0.5s) to avoid
 * running expensive sphere overlaps every frame.
 */
UCLASS(ClassGroup=(MC2), meta=(BlueprintSpawnableComponent))
class MECHCOMMANDER2_API USensorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USensorComponent();

	// --- Config ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sensor")
	float SensorRange = 2000.f;     // base detection range in UU (~200m at 10 UU/m scale)

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sensor")
	float LOSRange = 1500.f;        // visual (line-of-sight) range

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sensor")
	bool bHasECM = false;           // this unit projects an ECM bubble

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sensor")
	float ECMRadius = 500.f;        // ECM jams enemy sensors within this radius

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sensor")
	float ECMRangeReduction = 0.5f; // enemy sensor range multiplier when jammed

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sensor")
	float SensorTickInterval = 0.5f;

	// --- Contacts ---

	UPROPERTY(BlueprintReadOnly, Category = "Sensor")
	TArray<FMC2ContactInfo> Contacts;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Sensor")
	AMC2Mover* GetNearestEnemy() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Sensor")
	bool HasContact(AMC2Mover* Unit) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Sensor")
	bool CanSeeUnit(AMC2Mover* Unit) const;

	// Fog of War: updates the game state's visibility grid for this unit's team.
	UPROPERTY(EditDefaultsOnly, Category = "FogOfWar")
	int32 FogOfWarTileSize = 100;   // UU per tile

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	float TimeSinceLastScan = 0.f;

	void RunSensorScan();
	bool CheckLineOfSight(const FVector& From, const FVector& To) const;
	float EffectiveSensorRange(AMC2Mover* Target) const;
	void UpdateFogOfWar();
};
