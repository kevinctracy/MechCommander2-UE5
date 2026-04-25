#include "AI/Tasks/BTTask_MoveToAttackRange.h"
#include "AI/MC2AIController.h"
#include "Units/MC2Mover.h"
#include "Units/WeaponHardpointComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_MoveToAttackRange::UBTTask_MoveToAttackRange()
{
	NodeName = TEXT("Move To Attack Range");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_MoveToAttackRange::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AMC2AIController* Controller = Cast<AMC2AIController>(OwnerComp.GetAIOwner());
	if (!Controller) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AMC2Mover* Target = Cast<AMC2Mover>(BB ? BB->GetValueAsObject(BB_TARGET_ACTOR) : nullptr);
	if (!Target || Target->bIsDestroyed) return EBTNodeResult::Failed;

	AMC2Mover* Mover = Cast<AMC2Mover>(Controller->GetPawn());
	if (!Mover || Mover->bImmobilized) return EBTNodeResult::Failed;

	// Determine effective weapon range from longest-ranged ready hardpoint
	float MaxRange = 500.f;  // fallback melee-ish range
	for (UWeaponHardpointComponent* WC : Mover->WeaponHardpoints)
	{
		if (WC && !WC->bDestroyed)
			MaxRange = FMath::Max(MaxRange, WC->WeaponRange);
	}

	float StopDistance = FMath::Max(MaxRange - RangeBuffer, 100.f);
	float DistSq = FVector::DistSquared(Mover->GetActorLocation(), Target->GetActorLocation());

	if (DistSq <= StopDistance * StopDistance)
		return EBTNodeResult::Succeeded;   // already in range

	FAIMoveRequest MoveReq(Target);
	MoveReq.SetAcceptanceRadius(StopDistance);
	MoveReq.SetUsePathfinding(true);
	Controller->MoveTo(MoveReq);
	bIsMoving = true;

	return EBTNodeResult::InProgress;
}

void UBTTask_MoveToAttackRange::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AMC2AIController* Controller = Cast<AMC2AIController>(OwnerComp.GetAIOwner());
	if (!Controller) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AMC2Mover* Target = Cast<AMC2Mover>(BB ? BB->GetValueAsObject(BB_TARGET_ACTOR) : nullptr);
	if (!Target || Target->bIsDestroyed) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

	AMC2Mover* Mover = Cast<AMC2Mover>(Controller->GetPawn());
	if (!Mover) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

	float MaxRange = 500.f;
	for (UWeaponHardpointComponent* WC : Mover->WeaponHardpoints)
		if (WC && !WC->bDestroyed)
			MaxRange = FMath::Max(MaxRange, WC->WeaponRange);

	float StopDistSq = FMath::Square(FMath::Max(MaxRange - RangeBuffer, 100.f));
	if (FVector::DistSquared(Mover->GetActorLocation(), Target->GetActorLocation()) <= StopDistSq)
	{
		Controller->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UBTTask_MoveToAttackRange::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AMC2AIController* Controller = Cast<AMC2AIController>(OwnerComp.GetAIOwner()))
		Controller->StopMovement();
	bIsMoving = false;
	return EBTNodeResult::Aborted;
}
