#include "Mission/MC2MissionVolume.h"
#include "Units/MC2Mover.h"

AMC2MissionVolume::AMC2MissionVolume()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMC2MissionVolume::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	AMC2Mover* Mover = Cast<AMC2Mover>(OtherActor);
	if (!Mover) return;
	if (Mover->bIsDestroyed) return;
	if (FilterTeamIndex >= 0 && Mover->TeamIndex != FilterTeamIndex) return;

	UnitsInside.AddUnique(Mover);
	OnMoverEntered.Broadcast(this, Mover);
	CheckTeamThreshold(Mover->TeamIndex);
}

void AMC2MissionVolume::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	AMC2Mover* Mover = Cast<AMC2Mover>(OtherActor);
	if (!Mover) return;

	UnitsInside.Remove(Mover);
	OnMoverExited.Broadcast(this, Mover);
}

int32 AMC2MissionVolume::GetUnitCountInside(int32 TeamIndex) const
{
	int32 Count = 0;
	for (const TObjectPtr<AMC2Mover>& M : UnitsInside)
	{
		if (M && !M->bIsDestroyed && (TeamIndex < 0 || M->TeamIndex == TeamIndex))
			++Count;
	}
	return Count;
}

bool AMC2MissionVolume::IsUnitInside(AMC2Mover* Mover) const
{
	for (const TObjectPtr<AMC2Mover>& M : UnitsInside)
		if (M == Mover) return true;
	return false;
}

void AMC2MissionVolume::CheckTeamThreshold(int32 TeamIndex)
{
	if (GetUnitCountInside(TeamIndex) >= RequiredUnitCount)
		OnTeamFullyInside.Broadcast(TeamIndex);
}
