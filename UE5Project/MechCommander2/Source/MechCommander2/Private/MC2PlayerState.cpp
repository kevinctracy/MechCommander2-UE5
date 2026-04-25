#include "MC2PlayerState.h"
#include "Net/UnrealNetwork.h"

void AMC2PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMC2PlayerState, TeamIndex);
	DOREPLIFETIME(AMC2PlayerState, bIsReady);
	DOREPLIFETIME(AMC2PlayerState, CommanderName);
	DOREPLIFETIME(AMC2PlayerState, MissionKills);
	DOREPLIFETIME(AMC2PlayerState, MissionLosses);
}
