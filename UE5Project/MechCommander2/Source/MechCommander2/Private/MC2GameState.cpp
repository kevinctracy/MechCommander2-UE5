#include "MC2GameState.h"
#include "Net/UnrealNetwork.h"

static const int32 NUM_SCENARIO_TIMERS = 8;
static const int32 NUM_MISSION_FLAGS   = 64;

AMC2GameState::AMC2GameState()
{
	PrimaryActorTick.bCanEverTick = true;

	ScenarioTimers.Init(0.f, NUM_SCENARIO_TIMERS);
	TimerActive.Init(false, NUM_SCENARIO_TIMERS);
	MissionFlags.Init(false, NUM_MISSION_FLAGS);
	FogOfWarBits.Init(0, (FOW_DIM * FOW_DIM * FOW_TEAMS + 7) / 8);
}

void AMC2GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMC2GameState, MissionResult);
	DOREPLIFETIME(AMC2GameState, ScenarioTimers);
	DOREPLIFETIME(AMC2GameState, TimerActive);
	DOREPLIFETIME(AMC2GameState, MissionFlags);
	DOREPLIFETIME(AMC2GameState, Teams);
}

void AMC2GameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
		return;

	for (int32 i = 0; i < NUM_SCENARIO_TIMERS; ++i)
	{
		if (TimerActive[i])
			ScenarioTimers[i] += DeltaSeconds;
	}
}

void AMC2GameState::StartTimer(int32 TimerIndex)
{
	if (TimerIndex >= 0 && TimerIndex < NUM_SCENARIO_TIMERS)
		TimerActive[TimerIndex] = true;
}

void AMC2GameState::StopTimer(int32 TimerIndex)
{
	if (TimerIndex >= 0 && TimerIndex < NUM_SCENARIO_TIMERS)
		TimerActive[TimerIndex] = false;
}

float AMC2GameState::GetTimerValue(int32 TimerIndex) const
{
	return (TimerIndex >= 0 && TimerIndex < ScenarioTimers.Num()) ? ScenarioTimers[TimerIndex] : 0.f;
}

void AMC2GameState::SetMissionFlag(int32 FlagIndex, bool Value)
{
	if (FlagIndex >= 0 && FlagIndex < NUM_MISSION_FLAGS)
		MissionFlags[FlagIndex] = Value;
}

bool AMC2GameState::GetMissionFlag(int32 FlagIndex) const
{
	return (FlagIndex >= 0 && FlagIndex < MissionFlags.Num()) ? MissionFlags[FlagIndex] : false;
}

void AMC2GameState::RegisterTeam(int32 TeamIndex, const FString& TeamName)
{
	while (Teams.Num() <= TeamIndex)
		Teams.AddDefaulted();
	Teams[TeamIndex].TeamIndex = TeamIndex;
	Teams[TeamIndex].TeamName  = TeamName;
}

void AMC2GameState::OnUnitDestroyed(int32 TeamIndex)
{
	if (TeamIndex >= 0 && TeamIndex < Teams.Num())
	{
		Teams[TeamIndex].UnitsAlive  = FMath::Max(0, Teams[TeamIndex].UnitsAlive - 1);
		Teams[TeamIndex].UnitsLost  += 1;
	}
}

bool AMC2GameState::IsTeamDefeated(int32 TeamIndex) const
{
	return (TeamIndex >= 0 && TeamIndex < Teams.Num()) && Teams[TeamIndex].UnitsAlive <= 0;
}

void AMC2GameState::SetTileVisible(int32 TileX, int32 TileY, int32 TeamIndex, bool bVisible)
{
	if (TileX < 0 || TileX >= FOW_DIM || TileY < 0 || TileY >= FOW_DIM || TeamIndex < 0 || TeamIndex >= FOW_TEAMS)
		return;
	int32 BitIdx = FowBitIndex(TileX, TileY, TeamIndex);
	int32 ByteIdx = BitIdx / 8;
	int32 BitOff  = BitIdx % 8;
	if (bVisible)
		FogOfWarBits[ByteIdx] |=  (1 << BitOff);
	else
		FogOfWarBits[ByteIdx] &= ~(1 << BitOff);
}

bool AMC2GameState::IsTileVisible(int32 TileX, int32 TileY, int32 TeamIndex) const
{
	if (TileX < 0 || TileX >= FOW_DIM || TileY < 0 || TileY >= FOW_DIM || TeamIndex < 0 || TeamIndex >= FOW_TEAMS)
		return false;
	int32 BitIdx = FowBitIndex(TileX, TileY, TeamIndex);
	return (FogOfWarBits[BitIdx / 8] >> (BitIdx % 8)) & 1;
}

void AMC2GameState::OnRep_MissionResult()
{
	OnMissionResultChanged(MissionResult);
}

void AMC2GameState::Multicast_OnMissionReady_Implementation(int32 TotalUnits)
{
	bMissionReady = true;
	OnMissionReady(TotalUnits);
}
