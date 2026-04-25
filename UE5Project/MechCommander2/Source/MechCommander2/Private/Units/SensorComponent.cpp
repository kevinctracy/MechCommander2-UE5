#include "Units/SensorComponent.h"
#include "Units/MC2Mover.h"
#include "MC2GameState.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

USensorComponent::USensorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(false);  // sensor state is server-only
}

void USensorComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USensorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner()->HasAuthority())
		return;

	TimeSinceLastScan += DeltaTime;
	if (TimeSinceLastScan >= SensorTickInterval)
	{
		TimeSinceLastScan = 0.f;
		RunSensorScan();
		UpdateFogOfWar();
	}
}

void USensorComponent::RunSensorScan()
{
	AMC2Mover* OwnerMover = Cast<AMC2Mover>(GetOwner());
	if (!OwnerMover)
		return;

	const FVector OwnerLoc = OwnerMover->GetActorLocation();
	const int32 OwnerTeam  = OwnerMover->TeamIndex;

	// Sphere overlap to find candidate units
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());
	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		OwnerLoc,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(SensorRange * 1.1f),  // slightly larger than max range
		Params
	);

	Contacts.Reset();
	const float Now = GetWorld()->GetTimeSeconds();

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AMC2Mover* Target = Cast<AMC2Mover>(Overlap.GetActor());
		if (!Target || Target->TeamIndex == OwnerTeam || Target->bIsDestroyed)
			continue;

		float EffRange = EffectiveSensorRange(Target);
		float DistSq   = FVector::DistSquared(OwnerLoc, Target->GetActorLocation());

		if (DistSq > FMath::Square(EffRange))
			continue;

		FMC2ContactInfo Contact;
		Contact.Unit         = Target;
		Contact.DistanceSq   = DistSq;
		Contact.LastSeenTime = Now;
		Contact.bHasLineOfSight = (DistSq <= FMath::Square(LOSRange))
		                       && CheckLineOfSight(OwnerLoc, Target->GetActorLocation());

		Contacts.Add(Contact);
	}

	// Sort by distance (nearest first) for AI priority
	Contacts.Sort([](const FMC2ContactInfo& A, const FMC2ContactInfo& B)
	{
		return A.DistanceSq < B.DistanceSq;
	});
}

bool USensorComponent::CheckLineOfSight(const FVector& From, const FVector& To) const
{
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());
	// Trace upward from ground level to avoid false misses
	FVector FromEye = From + FVector(0, 0, 150.f);
	FVector ToEye   = To   + FVector(0, 0, 150.f);
	return !GetWorld()->LineTraceSingleByChannel(Hit, FromEye, ToEye, ECC_Visibility, Params);
}

float USensorComponent::EffectiveSensorRange(AMC2Mover* Target) const
{
	if (!Target)
		return SensorRange;

	// Check if target has an ECM component that reduces our range
	USensorComponent* TargetSensor = Target->FindComponentByClass<USensorComponent>();
	if (TargetSensor && TargetSensor->bHasECM)
	{
		float DistToTarget = FVector::Dist(GetOwner()->GetActorLocation(), Target->GetActorLocation());
		if (DistToTarget <= TargetSensor->ECMRadius)
			return SensorRange * TargetSensor->ECMRangeReduction;
	}
	return SensorRange;
}

void USensorComponent::UpdateFogOfWar()
{
	AMC2Mover* OwnerMover = Cast<AMC2Mover>(GetOwner());
	AMC2GameState* GS = GetWorld()->GetGameState<AMC2GameState>();
	if (!OwnerMover || !GS)
		return;

	// Mark tiles visible around this unit within sensor range
	const FVector OwnerLoc = OwnerMover->GetActorLocation();
	int32 RadiusTiles = FMath::CeilToInt(SensorRange / FogOfWarTileSize);

	int32 CenterX = FMath::RoundToInt(OwnerLoc.X / FogOfWarTileSize);
	int32 CenterY = FMath::RoundToInt(OwnerLoc.Y / FogOfWarTileSize);

	for (int32 DX = -RadiusTiles; DX <= RadiusTiles; ++DX)
	{
		for (int32 DY = -RadiusTiles; DY <= RadiusTiles; ++DY)
		{
			if (DX*DX + DY*DY <= RadiusTiles * RadiusTiles)
				GS->SetTileVisible(CenterX + DX, CenterY + DY, OwnerMover->TeamIndex, true);
		}
	}
}

AMC2Mover* USensorComponent::GetNearestEnemy() const
{
	return Contacts.IsEmpty() ? nullptr : Contacts[0].Unit.Get();
}

bool USensorComponent::HasContact(AMC2Mover* Unit) const
{
	return Contacts.ContainsByPredicate([Unit](const FMC2ContactInfo& C)
	{
		return C.Unit.Get() == Unit;
	});
}

bool USensorComponent::CanSeeUnit(AMC2Mover* Unit) const
{
	for (const FMC2ContactInfo& C : Contacts)
	{
		if (C.Unit.Get() == Unit)
			return C.bHasLineOfSight;
	}
	return false;
}
