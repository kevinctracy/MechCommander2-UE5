#include "AI/Tasks/BTTask_FireAtTarget.h"
#include "AI/MC2AIController.h"
#include "Units/MC2Mover.h"
#include "Units/MC2PilotComponent.h"
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

	// Pilot skill modifier (P4.2.6): roll hit chance per-weapon.
	// Veterans (Gunnery 2-3) have higher hit chance than green pilots (Gunnery 5-6).
	// Range modifier: add 1 to effective gunnery per range band past optimal.
	UMC2PilotComponent* Pilot = Mover->FindComponentByClass<UMC2PilotComponent>();
	const float BaseHitChance = Pilot ? Pilot->GetBaseHitChance() : 0.58f;  // 4 gunnery default

	const FVector MoverLoc = Mover->GetActorLocation();
	const FVector TargetLoc = Target->GetActorLocation();
	const float Dist = FVector::Dist(MoverLoc, TargetLoc);

	for (UWeaponHardpointComponent* WC : Mover->WeaponHardpoints)
	{
		if (!WC || WC->bDestroyed) continue;
		if (WC->CooldownRemaining > 0.f) continue;
		if (WC->CurrentAmmo == 0) continue;
		if (Dist > WC->WeaponRange) continue;

		// Range band penalty: each full half-range beyond optimal loses 8% hit chance
		// (matches BT: +1 gunnery per medium/long range band = ~8% hit reduction)
		const float OptimalRange = WC->WeaponRange * 0.5f;
		const int32 RangePenaltyBands = (Dist > OptimalRange)
			? FMath::FloorToInt((Dist - OptimalRange) / (OptimalRange * 0.5f))
			: 0;
		const float FinalHitChance = FMath::Clamp(BaseHitChance - RangePenaltyBands * 0.08f, 0.05f, 1.f);

		if (FMath::FRand() <= FinalHitChance)
			WC->FireAt(Target);
		// On a miss the weapon still fires visually (projectile spawned by FireAt with bMiss flag),
		// cooldown still consumed — matching MC2 behavior where fire animations always play.
	}

	return EBTNodeResult::Succeeded;
}
