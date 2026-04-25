#include "Mission/MC2Objective.h"
#include "Mission/MC2GameMode.h"
#include "Units/MC2Mover.h"
#include "Engine/TriggerVolume.h"
#include "Components/ShapeComponent.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace
{
	// Enemy = any team that is not team 0 (player team)
	constexpr int32 PLAYER_TEAM = 0;

	bool IsEnemy(const AMC2Mover* Unit)
	{
		return Unit && Unit->TeamIndex != PLAYER_TEAM;
	}

	bool IsAlive(const AMC2Mover* Unit)
	{
		return Unit && !Unit->bIsDestroyed;
	}

	bool IsDeadOrFled(const AMC2Mover* Unit, float BoundaryRadius)
	{
		if (!Unit) return true;
		if (Unit->bIsDestroyed) return true;
		// "Fled" = outside the circular battlefield boundary
		return Unit->GetActorLocation().Size2D() > BoundaryRadius;
	}

	bool IsCaptured(const AMC2Mover* Unit)
	{
		// MC2 "capture" means a unit has been salvaged (team index changed to player)
		return Unit && Unit->TeamIndex == PLAYER_TEAM;
	}

	bool IsInsideTrigger(const AActor* Actor, ATriggerVolume* Trigger)
	{
		if (!Actor || !Trigger) return false;
		UShapeComponent* Shape = Cast<UShapeComponent>(Trigger->GetRootComponent());
		return Shape && Shape->IsOverlappingActor(Actor);
	}
}

// ---------------------------------------------------------------------------
// DESTROY group (5 conditions)
// ---------------------------------------------------------------------------

EMC2ObjectiveStatus UObj_DestroyAllEnemy::Evaluate(AMC2GameMode* GameMode)
{
	for (AMC2Mover* Unit : GameMode->GetAllUnits())
	{
		if (IsEnemy(Unit) && IsAlive(Unit))
			return EMC2ObjectiveStatus::Incomplete;
	}
	return EMC2ObjectiveStatus::Complete;
}

EMC2ObjectiveStatus UObj_DestroyNumberOfEnemy::Evaluate(AMC2GameMode* GameMode)
{
	int32 Count = 0;
	for (AMC2Mover* Unit : GameMode->GetAllUnits())
	{
		if (IsEnemy(Unit) && Unit->bIsDestroyed)
			Count++;
	}
	DestroyedCount = Count;
	return (Count >= RequiredCount) ? EMC2ObjectiveStatus::Complete : EMC2ObjectiveStatus::Incomplete;
}

EMC2ObjectiveStatus UObj_DestroyEnemyGroup::Evaluate(AMC2GameMode* GameMode)
{
	for (AMC2Mover* Unit : GameMode->GetAllUnits())
	{
		if (!Unit || Unit->TeamIndex == PLAYER_TEAM) continue;
		if (Unit->Tags.Contains(GroupTag) && IsAlive(Unit))
			return EMC2ObjectiveStatus::Incomplete;
	}
	return EMC2ObjectiveStatus::Complete;
}

EMC2ObjectiveStatus UObj_DestroySpecificEnemy::Evaluate(AMC2GameMode* GameMode)
{
	AMC2Mover* Target = TargetUnit.Get();
	if (!Target) return EMC2ObjectiveStatus::Complete; // gone = destroyed
	return Target->bIsDestroyed ? EMC2ObjectiveStatus::Complete : EMC2ObjectiveStatus::Incomplete;
}

EMC2ObjectiveStatus UObj_DestroySpecificStructure::Evaluate(AMC2GameMode* GameMode)
{
	AActor* Target = TargetStructure.Get();
	if (!Target) return EMC2ObjectiveStatus::Complete;
	// Structures set bIsDestroyed via their own component; check generic IsActorBeingDestroyed
	return Target->IsActorBeingDestroyed() ? EMC2ObjectiveStatus::Complete : EMC2ObjectiveStatus::Incomplete;
}

// ---------------------------------------------------------------------------
// CAPTURE_OR_DESTROY group (5 conditions)
// ---------------------------------------------------------------------------

EMC2ObjectiveStatus UObj_CaptureOrDestroyAllEnemy::Evaluate(AMC2GameMode* GameMode)
{
	for (AMC2Mover* Unit : GameMode->GetAllUnits())
	{
		if (!IsEnemy(Unit)) continue;
		// Must be destroyed OR flipped to player team
		if (IsAlive(Unit) && Unit->TeamIndex != PLAYER_TEAM)
			return EMC2ObjectiveStatus::Incomplete;
	}
	return EMC2ObjectiveStatus::Complete;
}

EMC2ObjectiveStatus UObj_CaptureOrDestroyNumber::Evaluate(AMC2GameMode* GameMode)
{
	int32 Count = 0;
	for (AMC2Mover* Unit : GameMode->GetAllUnits())
	{
		if (!Unit) continue;
		// Originally enemy (we can't know original team at eval time, so track by destruction or capture)
		if (Unit->bIsDestroyed || IsCaptured(Unit))
			Count++;
	}
	CountAchieved = Count;
	return (Count >= RequiredCount) ? EMC2ObjectiveStatus::Complete : EMC2ObjectiveStatus::Incomplete;
}

EMC2ObjectiveStatus UObj_CaptureOrDestroyGroup::Evaluate(AMC2GameMode* GameMode)
{
	for (AMC2Mover* Unit : GameMode->GetAllUnits())
	{
		if (!Unit || !Unit->Tags.Contains(GroupTag)) continue;
		if (IsAlive(Unit) && !IsCaptured(Unit))
			return EMC2ObjectiveStatus::Incomplete;
	}
	return EMC2ObjectiveStatus::Complete;
}

EMC2ObjectiveStatus UObj_CaptureOrDestroySpecificEnemy::Evaluate(AMC2GameMode* GameMode)
{
	AMC2Mover* Target = TargetUnit.Get();
	if (!Target) return EMC2ObjectiveStatus::Complete;
	if (Target->bIsDestroyed || IsCaptured(Target))
		return EMC2ObjectiveStatus::Complete;
	return EMC2ObjectiveStatus::Incomplete;
}

EMC2ObjectiveStatus UObj_CaptureOrDestroySpecificStructure::Evaluate(AMC2GameMode* GameMode)
{
	AActor* Target = TargetStructure.Get();
	if (!Target) return EMC2ObjectiveStatus::Complete;
	return Target->IsActorBeingDestroyed() ? EMC2ObjectiveStatus::Complete : EMC2ObjectiveStatus::Incomplete;
}

// ---------------------------------------------------------------------------
// DEAD_OR_FLED group (4 conditions)
// ---------------------------------------------------------------------------

EMC2ObjectiveStatus UObj_DeadOrFledAllEnemy::Evaluate(AMC2GameMode* GameMode)
{
	for (AMC2Mover* Unit : GameMode->GetAllUnits())
	{
		if (!IsEnemy(Unit)) continue;
		if (!IsDeadOrFled(Unit, MapBoundaryRadius))
			return EMC2ObjectiveStatus::Incomplete;
	}
	return EMC2ObjectiveStatus::Complete;
}

EMC2ObjectiveStatus UObj_DeadOrFledNumber::Evaluate(AMC2GameMode* GameMode)
{
	int32 Count = 0;
	for (AMC2Mover* Unit : GameMode->GetAllUnits())
	{
		if (IsEnemy(Unit) && IsDeadOrFled(Unit, MapBoundaryRadius))
			Count++;
	}
	return (Count >= RequiredCount) ? EMC2ObjectiveStatus::Complete : EMC2ObjectiveStatus::Incomplete;
}

EMC2ObjectiveStatus UObj_DeadOrFledGroup::Evaluate(AMC2GameMode* GameMode)
{
	for (AMC2Mover* Unit : GameMode->GetAllUnits())
	{
		if (!Unit || !Unit->Tags.Contains(GroupTag)) continue;
		if (!IsDeadOrFled(Unit, MapBoundaryRadius))
			return EMC2ObjectiveStatus::Incomplete;
	}
	return EMC2ObjectiveStatus::Complete;
}

EMC2ObjectiveStatus UObj_DeadOrFledSpecific::Evaluate(AMC2GameMode* GameMode)
{
	AMC2Mover* Target = TargetUnit.Get();
	if (!Target) return EMC2ObjectiveStatus::Complete;
	return IsDeadOrFled(Target, MapBoundaryRadius) ? EMC2ObjectiveStatus::Complete : EMC2ObjectiveStatus::Incomplete;
}

// ---------------------------------------------------------------------------
// CAPTURE group (2 conditions)
// ---------------------------------------------------------------------------

EMC2ObjectiveStatus UObj_CaptureUnit::Evaluate(AMC2GameMode* GameMode)
{
	AMC2Mover* Target = TargetUnit.Get();
	if (!Target || Target->bIsDestroyed) return EMC2ObjectiveStatus::Failed;

	// Find the nearest player unit and check proximity
	TArray<AMC2Mover*> PlayerUnits = GameMode->GetUnitsByTeam(PLAYER_TEAM);
	for (AMC2Mover* PUnit : PlayerUnits)
	{
		if (!PUnit || PUnit->bIsDestroyed) continue;
		if (FVector::DistSquared(PUnit->GetActorLocation(), Target->GetActorLocation()) <= CaptureRadius * CaptureRadius)
			return EMC2ObjectiveStatus::Complete;
	}
	return EMC2ObjectiveStatus::Incomplete;
}

EMC2ObjectiveStatus UObj_CaptureStructure::Evaluate(AMC2GameMode* GameMode)
{
	AActor* Target = TargetStructure.Get();
	if (!Target || Target->IsActorBeingDestroyed()) return EMC2ObjectiveStatus::Failed;

	TArray<AMC2Mover*> PlayerUnits = GameMode->GetUnitsByTeam(PLAYER_TEAM);
	for (AMC2Mover* PUnit : PlayerUnits)
	{
		if (!PUnit || PUnit->bIsDestroyed) continue;
		if (FVector::DistSquared(PUnit->GetActorLocation(), Target->GetActorLocation()) <= CaptureRadius * CaptureRadius)
			return EMC2ObjectiveStatus::Complete;
	}
	return EMC2ObjectiveStatus::Incomplete;
}

// ---------------------------------------------------------------------------
// GUARD group (2 conditions)
// ---------------------------------------------------------------------------

EMC2ObjectiveStatus UObj_GuardUnit::Evaluate(AMC2GameMode* GameMode)
{
	AMC2Mover* Target = GuardedUnit.Get();
	if (!Target) return EMC2ObjectiveStatus::Failed;
	// Guard fails if the guarded unit is destroyed; success is maintained as long as alive
	return Target->bIsDestroyed ? EMC2ObjectiveStatus::Failed : EMC2ObjectiveStatus::Incomplete;
}

EMC2ObjectiveStatus UObj_GuardStructure::Evaluate(AMC2GameMode* GameMode)
{
	AActor* Target = GuardedStructure.Get();
	if (!Target) return EMC2ObjectiveStatus::Failed;
	return Target->IsActorBeingDestroyed() ? EMC2ObjectiveStatus::Failed : EMC2ObjectiveStatus::Incomplete;
}

// ---------------------------------------------------------------------------
// MOVE group (4 conditions)
// ---------------------------------------------------------------------------

EMC2ObjectiveStatus UObj_MoveAnyUnitToArea::Evaluate(AMC2GameMode* GameMode)
{
	ATriggerVolume* Trigger = TriggerArea.Get();
	if (!Trigger) return EMC2ObjectiveStatus::Failed;

	TArray<AMC2Mover*> PlayerUnits = GameMode->GetUnitsByTeam(PLAYER_TEAM);
	for (AMC2Mover* Unit : PlayerUnits)
	{
		if (IsAlive(Unit) && IsInsideTrigger(Unit, Trigger))
			return EMC2ObjectiveStatus::Complete;
	}
	return EMC2ObjectiveStatus::Incomplete;
}

EMC2ObjectiveStatus UObj_MoveAllUnitsToArea::Evaluate(AMC2GameMode* GameMode)
{
	ATriggerVolume* Trigger = TriggerArea.Get();
	if (!Trigger) return EMC2ObjectiveStatus::Failed;

	TArray<AMC2Mover*> PlayerUnits = GameMode->GetUnitsByTeam(PLAYER_TEAM);
	if (PlayerUnits.IsEmpty()) return EMC2ObjectiveStatus::Incomplete;

	for (AMC2Mover* Unit : PlayerUnits)
	{
		if (!IsInsideTrigger(Unit, Trigger))
			return EMC2ObjectiveStatus::Incomplete;
	}
	return EMC2ObjectiveStatus::Complete;
}

EMC2ObjectiveStatus UObj_MoveAllSurvivingUnitsToArea::Evaluate(AMC2GameMode* GameMode)
{
	ATriggerVolume* Trigger = TriggerArea.Get();
	if (!Trigger) return EMC2ObjectiveStatus::Failed;

	TArray<AMC2Mover*> PlayerUnits = GameMode->GetUnitsByTeam(PLAYER_TEAM);
	bool bAnyAlive = false;
	for (AMC2Mover* Unit : PlayerUnits)
	{
		if (!IsAlive(Unit)) continue;
		bAnyAlive = true;
		if (!IsInsideTrigger(Unit, Trigger))
			return EMC2ObjectiveStatus::Incomplete;
	}
	return bAnyAlive ? EMC2ObjectiveStatus::Complete : EMC2ObjectiveStatus::Incomplete;
}

EMC2ObjectiveStatus UObj_MoveAllSurvivingMechsToArea::Evaluate(AMC2GameMode* GameMode)
{
	ATriggerVolume* Trigger = TriggerArea.Get();
	if (!Trigger) return EMC2ObjectiveStatus::Failed;

	TArray<AMC2Mover*> PlayerUnits = GameMode->GetUnitsByTeam(PLAYER_TEAM);
	bool bAnyMech = false;
	for (AMC2Mover* Unit : PlayerUnits)
	{
		if (!IsAlive(Unit)) continue;
		// Mechs carry the "Mech" tag set in their Blueprint class defaults
		if (!Unit->Tags.Contains(FName("Mech"))) continue;
		bAnyMech = true;
		if (!IsInsideTrigger(Unit, Trigger))
			return EMC2ObjectiveStatus::Incomplete;
	}
	return bAnyMech ? EMC2ObjectiveStatus::Complete : EMC2ObjectiveStatus::Incomplete;
}

// ---------------------------------------------------------------------------
// STATE conditions (2 conditions)
// ---------------------------------------------------------------------------

EMC2ObjectiveStatus UObj_BooleanFlagIsSet::Evaluate(AMC2GameMode* GameMode)
{
	bool CurrentValue = GameMode->ScriptGetMissionFlag(FlagIndex);
	return (CurrentValue == RequiredValue) ? EMC2ObjectiveStatus::Complete : EMC2ObjectiveStatus::Incomplete;
}

EMC2ObjectiveStatus UObj_ElapsedMissionTime::Evaluate(AMC2GameMode* GameMode)
{
	float Elapsed = GameMode->ScriptGetTimer(TimerIndex);
	return (Elapsed >= RequiredSeconds) ? EMC2ObjectiveStatus::Complete : EMC2ObjectiveStatus::Incomplete;
}
