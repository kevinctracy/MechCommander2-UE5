#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Patrol.generated.h"

class AMC2WaypointActor;

/**
 * UBTTask_Patrol
 * Moves through a chain of AMC2WaypointActor actors in order, looping.
 * The first waypoint is set via the WaypointStart reference on the controller.
 * Succeeds when the current waypoint is reached; caller loops the task.
 *
 * Maps to MC2's patrol order handling in warrior.cpp.
 */
UCLASS()
class MECHCOMMANDER2_API UBTTask_Patrol : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_Patrol();

	// Radius within which a waypoint counts as "reached"
	UPROPERTY(EditAnywhere, Category = "AI")
	float AcceptanceRadius = 200.f;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
