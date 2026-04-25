#include "AI/MC2AIController.h"
#include "Units/MC2Mover.h"
#include "Units/MC2HealthComponent.h"
#include "Units/SensorComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

AMC2AIController::AMC2AIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMC2AIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledMover = Cast<AMC2Mover>(InPawn);
	if (!ControlledMover.IsValid())
		return;

	UBehaviorTree* BT = (ControlledMover->TeamIndex == 0) ? AllyBehaviorTree : EnemyBehaviorTree;
	if (BT)
		RunBehaviorTree(BT);

	// Initialize home position so guard behavior knows where to return
	if (Blackboard)
		Blackboard->SetValueAsVector(BB_HOME_POSITION, InPawn->GetActorLocation());

	// Update blackboard every 0.25s (cheaper than every frame)
	GetWorldTimerManager().SetTimer(
		BlackboardUpdateTimer,
		this, &AMC2AIController::UpdateBlackboard,
		0.25f, true
	);
}

void AMC2AIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(BlackboardUpdateTimer);
	Super::OnUnPossess();
}

void AMC2AIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AMC2AIController::UpdateBlackboard()
{
	if (!Blackboard || !ControlledMover.IsValid())
		return;

	AMC2Mover* Mover = ControlledMover.Get();

	// Heat and shutdown state
	Blackboard->SetValueAsFloat(BB_CURRENT_HEAT, Mover->CurrentHeat / Mover->MaxHeat);
	Blackboard->SetValueAsBool (BB_IS_SHUTDOWN,  Mover->bShutDown);

	// Nearest enemy from sensor
	USensorComponent* Sensor = Mover->FindComponentByClass<USensorComponent>();
	if (Sensor)
	{
		AMC2Mover* NearestEnemy = Sensor->GetNearestEnemy();
		Blackboard->SetValueAsObject(BB_NEAREST_ENEMY, NearestEnemy);

		if (NearestEnemy)
		{
			Blackboard->SetValueAsObject(BB_TARGET_ACTOR, NearestEnemy);
			Blackboard->SetValueAsBool(BB_HAS_LOS, Sensor->CanSeeUnit(NearestEnemy));
		}
		else
		{
			Blackboard->SetValueAsObject(BB_TARGET_ACTOR, nullptr);
			Blackboard->SetValueAsBool(BB_HAS_LOS, false);
		}
	}

	// Current order target from mover
	const FMC2MoverOrder& Order = Mover->GetCurrentOrder();
	if (Order.OrderType != EMC2OrderType::None)
	{
		Blackboard->SetValueAsVector(BB_MOVE_TARGET, Order.TargetLocation);
		Blackboard->SetValueAsEnum  (BB_ORDER_TYPE,  (uint8)Order.OrderType);
	}
}
