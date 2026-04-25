#include "AI/Tasks/BTTask_Patrol.h"
#include "AI/MC2AIController.h"
#include "AI/MC2WaypointActor.h"
#include "Units/MC2Mover.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_Patrol::UBTTask_Patrol()
{
	NodeName = TEXT("Patrol Waypoints");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_Patrol::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AMC2AIController* Controller = Cast<AMC2AIController>(OwnerComp.GetAIOwner());
	if (!Controller) return EBTNodeResult::Failed;

	AMC2Mover* Mover = Cast<AMC2Mover>(Controller->GetPawn());
	if (!Mover || Mover->bImmobilized || Mover->bIsDestroyed) return EBTNodeResult::Failed;

	// Find current patrol waypoint stored in blackboard (or start from first tag match)
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AMC2WaypointActor* CurrentWP = BB ? Cast<AMC2WaypointActor>(BB->GetValueAsObject(FName("CurrentWaypoint"))) : nullptr;

	// If no waypoint set, find nearest one tagged for this unit
	if (!CurrentWP)
	{
		UWorld* World = Controller->GetWorld();
		float BestDist = FLT_MAX;
		for (TActorIterator<AMC2WaypointActor> It(World); It; ++It)
		{
			float D = FVector::Dist(Mover->GetActorLocation(), (*It)->GetActorLocation());
			if (D < BestDist) { BestDist = D; CurrentWP = *It; }
		}
		if (!CurrentWP) return EBTNodeResult::Failed;
		if (BB) BB->SetValueAsObject(FName("CurrentWaypoint"), CurrentWP);
	}

	FAIMoveRequest MoveReq(CurrentWP->GetActorLocation());
	MoveReq.SetAcceptanceRadius(AcceptanceRadius);
	Controller->MoveTo(MoveReq);

	return EBTNodeResult::InProgress;
}

void UBTTask_Patrol::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AMC2AIController* Controller = Cast<AMC2AIController>(OwnerComp.GetAIOwner());
	if (!Controller) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

	AMC2Mover* Mover = Cast<AMC2Mover>(Controller->GetPawn());
	if (!Mover) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AMC2WaypointActor* WP = BB ? Cast<AMC2WaypointActor>(BB->GetValueAsObject(FName("CurrentWaypoint"))) : nullptr;
	if (!WP) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

	float DistSq = FVector::DistSquared2D(Mover->GetActorLocation(), WP->GetActorLocation());
	if (DistSq <= AcceptanceRadius * AcceptanceRadius)
	{
		// Advance to next waypoint (loop if at end)
		AMC2WaypointActor* Next = WP->NextWaypoint ? WP->NextWaypoint : WP->GetChainStart();
		if (BB) BB->SetValueAsObject(FName("CurrentWaypoint"), Next);

		// Succeed so the BT can loop this task or wait at waypoint
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UBTTask_Patrol::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AMC2AIController* Controller = Cast<AMC2AIController>(OwnerComp.GetAIOwner()))
		Controller->StopMovement();
	return EBTNodeResult::Aborted;
}
