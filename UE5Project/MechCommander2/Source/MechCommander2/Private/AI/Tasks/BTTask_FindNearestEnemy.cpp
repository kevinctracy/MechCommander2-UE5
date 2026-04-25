#include "AI/Tasks/BTTask_FindNearestEnemy.h"
#include "AI/MC2AIController.h"
#include "Units/MC2Mover.h"
#include "Units/SensorComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_FindNearestEnemy::UBTTask_FindNearestEnemy()
{
	NodeName = TEXT("Find Nearest Enemy");
}

EBTNodeResult::Type UBTTask_FindNearestEnemy::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AMC2AIController* Controller = Cast<AMC2AIController>(OwnerComp.GetAIOwner());
	if (!Controller) return EBTNodeResult::Failed;

	AMC2Mover* Mover = Cast<AMC2Mover>(Controller->GetPawn());
	if (!Mover) return EBTNodeResult::Failed;

	USensorComponent* Sensor = Mover->FindComponentByClass<USensorComponent>();
	if (!Sensor) return EBTNodeResult::Failed;

	AMC2Mover* Best = nullptr;
	float BestDistSq = FLT_MAX;

	for (const FMC2ContactInfo& Contact : Sensor->GetContacts())
	{
		if (!Contact.Unit.IsValid() || Contact.Unit->bIsDestroyed) continue;
		if (bRequireLOS && !Contact.bHasLineOfSight) continue;

		if (Contact.DistanceSq < BestDistSq)
		{
			BestDistSq = Contact.DistanceSq;
			Best = Contact.Unit.Get();
		}
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	if (Best)
	{
		BB->SetValueAsObject(BB_NEAREST_ENEMY, Best);
		BB->SetValueAsObject(BB_TARGET_ACTOR,  Best);
		BB->SetValueAsBool(BB_HAS_LOS, Sensor->CanSeeUnit(Best));
		return EBTNodeResult::Succeeded;
	}

	BB->SetValueAsObject(BB_NEAREST_ENEMY, nullptr);
	BB->SetValueAsObject(BB_TARGET_ACTOR,  nullptr);
	return EBTNodeResult::Failed;
}
