#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FireAtTarget.generated.h"

/**
 * UBTTask_FireAtTarget
 * Fires all ready weapon hardpoints at BB_TARGET_ACTOR.
 * Respects heat threshold — stops firing if heat exceeds OverheatFireThreshold.
 * Always Succeeds (firing is a one-shot action, not a waiting task).
 *
 * Maps to MC2's warrior.cpp::fireWeapons().
 */
UCLASS()
class MECHCOMMANDER2_API UBTTask_FireAtTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FireAtTarget();

	// Heat% above which firing is suppressed (matches AMC2AIController::OverheatFireThreshold)
	UPROPERTY(EditAnywhere, Category = "AI")
	float HeatSuppressThreshold = 0.85f;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
