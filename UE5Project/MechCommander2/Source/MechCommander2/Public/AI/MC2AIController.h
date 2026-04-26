#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MC2AIController.generated.h"

class UBehaviorTree;
class UBehaviorTreeComponent;
class UBlackboardComponent;
class USensorComponent;
class AMC2Mover;

// Blackboard key names — shared with BT_EnemyMech Blueprint
#define BB_TARGET_ACTOR    TEXT("TargetActor")
#define BB_MOVE_TARGET     TEXT("MoveTarget")
#define BB_ORDER_TYPE      TEXT("OrderType")
#define BB_CURRENT_HEAT    TEXT("CurrentHeat")
#define BB_NEAREST_ENEMY   TEXT("NearestEnemy")
#define BB_HAS_LOS         TEXT("HasLOS")
#define BB_IS_SHUTDOWN     TEXT("IsShutDown")
#define BB_HOME_POSITION   TEXT("HomePosition")

/**
 * AMC2AIController
 * Controls enemy mechs and vehicles via Behavior Tree.
 * The BT has four root branches (priority order):
 *   1. Retreat when critically damaged
 *   2. Attack nearest sensor contact with weapons in range
 *   3. Execute current player-issued order (if player-controlled ally)
 *   4. Patrol waypoints / guard home position
 *
 * Player-controlled units also use this controller — orders come from
 * AMC2PlayerController which calls ReceiveMoveOrder / ReceiveAttackOrder
 * on AMC2Mover, which routes to this AI controller.
 */
UCLASS()
class MECHCOMMANDER2_API AMC2AIController : public AAIController
{
	GENERATED_BODY()

public:
	AMC2AIController();

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> EnemyBehaviorTree;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> AllyBehaviorTree;

	// Minimum health % before retreat behavior activates
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float RetreatHealthThreshold = 0.2f;

	// Heat % above which AI stops firing to cool down
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float OverheatFireThreshold = 0.85f;

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;

	// Called by the sensor component when contacts change
	UFUNCTION(BlueprintCallable, Category = "AI")
	void UpdateBlackboard();

	// --- Formation pathfinding (P4.1.3) ---
	// Assign this unit a slot in a formation relative to a leader mech.
	// The AI will target (LeaderDestination + Offset) instead of the raw move order location.
	// Offset is in the leader's local space at the time of the order — rotated each tick.
	UFUNCTION(BlueprintCallable, Category = "AI|Formation")
	void SetFormationSlot(AMC2Mover* Leader, FVector LocalOffset);

	UFUNCTION(BlueprintCallable, Category = "AI|Formation")
	void ClearFormation();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Formation")
	bool IsInFormation() const { return FormationLeader.IsValid(); }

private:
	TWeakObjectPtr<AMC2Mover> ControlledMover;
	FTimerHandle BlackboardUpdateTimer;

	// Formation state — set by player controller when issuing a group move order
	TWeakObjectPtr<AMC2Mover> FormationLeader;
	FVector FormationLocalOffset = FVector::ZeroVector;

	// Compute world-space formation goal from leader's current move destination
	FVector ComputeFormationTarget() const;
};
