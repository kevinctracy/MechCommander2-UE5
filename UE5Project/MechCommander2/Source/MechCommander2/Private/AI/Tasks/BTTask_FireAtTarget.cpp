#include "AI/Tasks/BTTask_FireAtTarget.h"
#include "AI/MC2AIController.h"
#include "Units/MC2Mover.h"
#include "Units/WeaponHardpointComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_FireAtTarget::UBTTask_FireAtTarget()
{
	NodeName = TEXT("Fire At Target");
}

EBTNodeResult::Type UBTTask_FireAtTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AMC2AIController* Controller = Cast<AMC2AIController>(OwnerComp.GetAIOwner());
	if (!Controller) return EBTNodeResult::Failed;

	AMC2Mover* Mover = Cast<AMC2Mover>(Controller->GetPawn());
	if (!Mover || Mover->bShutDown) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AActor* Target = BB ? Cast<AActor>(BB->GetValueAsObject(BB_TARGET_ACTOR)) : nullptr;
	if (!Target) return EBTNodeResult::Failed;

	// Suppress firing if overheating
	float HeatPct = (Mover->MaxHeat > 0.f) ? (Mover->CurrentHeat / Mover->MaxHeat) : 0.f;
	if (HeatPct >= HeatSuppressThreshold)
		return EBTNodeResult::Succeeded;  // task "succeeds" but we just don't fire

	// Turn torso to face target
	FVector ToTarget = (Target->GetActorLocation() - Mover->GetActorLocation()).GetSafeNormal();
	// Rotation is handled by the mover's aiming system; just issue fire commands
	for (UWeaponHardpointComponent* WC : Mover->WeaponHardpoints)
	{
		if (!WC || WC->bDestroyed) continue;
		if (WC->CooldownRemaining > 0.f) continue;
		if (WC->CurrentAmmo == 0) continue;

		float DistSq = FVector::DistSquared(Mover->GetActorLocation(), Target->GetActorLocation());
		if (DistSq > WC->WeaponRange * WC->WeaponRange) continue;

		WC->FireAt(Target);
	}

	return EBTNodeResult::Succeeded;
}
