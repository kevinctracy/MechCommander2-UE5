#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToAttackRange.generated.h"

/**
 * UBTTask_MoveToAttackRange
 * Moves the controlled mover to within firing range of BB_TARGET_ACTOR.
 * Reads the longest-range available weapon to determine stop distance.
 * Succeeds when within range, Fails if no target or path fails.
 *
 * Maps to MC2's warrior approach logic (warrior.cpp::approach()).
 */
UCLASS()
class MECHCOMMANDER2_API UBTTask_MoveToAttackRange : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MoveToAttackRange();

	// Extra buffer subtracted from weapon range so unit stops within comfortable firing distance
	UPROPERTY(EditAnywhere, Category = "AI")
	float RangeBuffer = 150.f;

	// Max distance for a path to be considered reachable before giving up
	UPROPERTY(EditAnywhere, Category = "AI")
	float MaxPathDistance = 50000.f;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	bool bIsMoving = false;
};
