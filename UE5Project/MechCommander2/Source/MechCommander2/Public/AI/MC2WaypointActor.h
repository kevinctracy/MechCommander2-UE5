#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MC2WaypointActor.generated.h"

/**
 * AMC2WaypointActor
 * Placed in the level to define patrol paths for enemy AI.
 * Chain waypoints via NextWaypoint; the BT patrol task follows the chain and loops.
 *
 * Maps to MC2's waypoint system referenced in mission PAK data.
 */
UCLASS(BlueprintType)
class MECHCOMMANDER2_API AMC2WaypointActor : public AActor
{
	GENERATED_BODY()

public:
	AMC2WaypointActor();

	// Next waypoint in the chain (nullptr = end of path, loop to first)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint")
	TObjectPtr<AMC2WaypointActor> NextWaypoint;

	// Optional wait time at this waypoint before moving on (0 = no wait)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint")
	float WaitDuration = 0.f;

	// Returns the start of the chain this waypoint belongs to
	UFUNCTION(BlueprintCallable, Category = "Waypoint")
	AMC2WaypointActor* GetChainStart();

#if WITH_EDITORONLY_DATA
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void BeginPlay() override;
};
