#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindNearestEnemy.generated.h"

/**
 * UBTTask_FindNearestEnemy
 * Queries the controlled mover's USensorComponent for the nearest enemy contact.
 * Writes result to BB_NEAREST_ENEMY and BB_TARGET_ACTOR blackboard keys.
 * Returns Succeeded if an enemy is found, Failed otherwise.
 *
 * Maps to MC2's contact resolution in warrior.cpp::findBestTarget().
 */
UCLASS()
class MECHCOMMANDER2_API UBTTask_FindNearestEnemy : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindNearestEnemy();

	// Minimum threat rating to consider a target (0 = any enemy)
	UPROPERTY(EditAnywhere, Category = "AI")
	float MinThreatRating = 0.f;

	// If true, only consider targets with line of sight
	UPROPERTY(EditAnywhere, Category = "AI")
	bool bRequireLOS = false;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
