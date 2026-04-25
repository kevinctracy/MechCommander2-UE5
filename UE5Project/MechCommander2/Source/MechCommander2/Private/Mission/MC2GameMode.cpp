#include "Mission/MC2GameMode.h"
#include "MC2GameState.h"
#include "MC2PlayerState.h"
#include "Units/MC2Mover.h"
#include "Units/MC2PilotComponent.h"
#include "Campaign/MC2LogisticsSubsystem.h"
#include "EngineUtils.h"

AMC2GameMode::AMC2GameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	GameStateClass = AMC2GameState::StaticClass();
}

void AMC2GameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
}

void AMC2GameMode::StartPlay()
{
	Super::StartPlay();
	SpawnAllParts();

	// Broadcast mission-ready signal to all clients (MCMSG_MissionSetup equivalent).
	// Clients use this to dismiss loading screens and activate HUD input.
	if (AMC2GameState* GS = GetGameState<AMC2GameState>())
		GS->Multicast_OnMissionReady(AllSpawnedUnits.Num());

	OnMissionStart();
}

void AMC2GameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bMissionOver)
		return;

	EvaluateObjectives();

	// Optional time limit
	if (MissionTimeLimit > 0.f)
	{
		AMC2GameState* GS = GetGameState<AMC2GameState>();
		if (GS && GS->GetTimerValue(0) >= MissionTimeLimit)
			ApplyMissionResult(EMC2MissionResult::Draw);
	}
}

// ---------------------------------------------------------------------------
// Spawning
// ---------------------------------------------------------------------------

void AMC2GameMode::SpawnAllParts()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	for (FMC2Part& Part : Parts)
	{
		TSubclassOf<AMC2Mover> ClassToSpawn = Part.UnitClass;
		if (!ClassToSpawn)
			continue;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AMC2Mover* Unit = World->SpawnActor<AMC2Mover>(ClassToSpawn, Part.SpawnTransform, Params);
		if (!Unit)
			continue;

		Unit->TeamIndex = Part.TeamIndex;
		AllSpawnedUnits.Add(Unit);

		// Update game state team tallies
		AMC2GameState* GS = GetGameState<AMC2GameState>();
		if (GS)
		{
			while (GS->Teams.Num() <= Part.TeamIndex)
			{
				FMC2TeamState TS;
				TS.TeamIndex = GS->Teams.Num();
				GS->Teams.Add(TS);
			}
			GS->Teams[Part.TeamIndex].UnitsAlive++;
		}
	}
}

AMC2Mover* AMC2GameMode::ScriptSpawnUnit(TSubclassOf<AMC2Mover> UnitClass, const FVector& Location, int32 Team)
{
	UWorld* World = GetWorld();
	if (!World || !UnitClass)
		return nullptr;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AMC2Mover* Unit = World->SpawnActor<AMC2Mover>(UnitClass, Location, FRotator::ZeroRotator, Params);
	if (Unit)
	{
		Unit->TeamIndex = Team;
		AllSpawnedUnits.Add(Unit);

		AMC2GameState* GS = GetGameState<AMC2GameState>();
		if (GS)
		{
			while (GS->Teams.Num() <= Team)
			{
				FMC2TeamState TS;
				TS.TeamIndex = GS->Teams.Num();
				GS->Teams.Add(TS);
			}
			GS->Teams[Team].UnitsAlive++;
		}
	}
	return Unit;
}

// ---------------------------------------------------------------------------
// Objective evaluation
// ---------------------------------------------------------------------------

void AMC2GameMode::EvaluateObjectives()
{
	bool bAllPrimaryComplete = true;
	bool bAnyPrimaryFailed   = false;

	for (UMC2Objective* Obj : PrimaryObjectives)
	{
		if (!Obj) continue;
		if (Obj->Status == EMC2ObjectiveStatus::Incomplete)
			Obj->Status = Obj->Evaluate(this);

		if (Obj->Status == EMC2ObjectiveStatus::Incomplete)
			bAllPrimaryComplete = false;
		else if (Obj->Status == EMC2ObjectiveStatus::Failed)
			bAnyPrimaryFailed = true;
	}

	// Evaluate secondary / bonus regardless of primary state
	for (UMC2Objective* Obj : SecondaryObjectives)
	{
		if (Obj && Obj->Status == EMC2ObjectiveStatus::Incomplete)
			Obj->Status = Obj->Evaluate(this);
	}
	for (UMC2Objective* Obj : BonusObjectives)
	{
		if (Obj && Obj->Status == EMC2ObjectiveStatus::Incomplete)
			Obj->Status = Obj->Evaluate(this);
	}

	if (bAnyPrimaryFailed)
	{
		ApplyMissionResult(EMC2MissionResult::PlayerLostBig);
	}
	else if (bAllPrimaryComplete && PrimaryObjectives.Num() > 0)
	{
		// Tally secondary/bonus to determine win magnitude
		bool bAllSecondary = SecondaryObjectives.Num() == 0;
		if (!bAllSecondary)
		{
			bAllSecondary = true;
			for (UMC2Objective* Obj : SecondaryObjectives)
				if (Obj && Obj->Status != EMC2ObjectiveStatus::Complete) { bAllSecondary = false; break; }
		}

		ApplyMissionResult(bAllSecondary ? EMC2MissionResult::PlayerWinBig : EMC2MissionResult::PlayerWinSmall);
	}
}

void AMC2GameMode::ApplyMissionResult(EMC2MissionResult Result)
{
	if (bMissionOver)
		return;

	bMissionOver = true;

	AMC2GameState* GS = GetGameState<AMC2GameState>();
	if (GS)
		GS->MissionResult = Result;

	// Commit pilot XP and record mission in the logistics subsystem
	CommitMissionToSave(Result);

	OnMissionResult(Result);
}

void AMC2GameMode::CommitMissionToSave(EMC2MissionResult Result)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UMC2LogisticsSubsystem* LS = GI->GetSubsystem<UMC2LogisticsSubsystem>();
	if (!LS) return;

	// Commit all pilot XP earned this mission
	for (AMC2Mover* Unit : AllSpawnedUnits)
	{
		if (!Unit || Unit->TeamIndex != 0) continue;
		if (UMC2PilotComponent* PC = Unit->FindComponentByClass<UMC2PilotComponent>())
			PC->CommitXPToSave();
	}

	// Count player kills / losses for the debrief stats
	int32 PlayerKills = 0, PlayerLosses = 0;
	for (AMC2Mover* Unit : AllSpawnedUnits)
	{
		if (!Unit) continue;
		if (Unit->TeamIndex != 0 && Unit->bIsDestroyed) PlayerKills++;
		if (Unit->TeamIndex == 0  && Unit->bIsDestroyed) PlayerLosses++;
	}

	// C-Bill rewards: win=50k base, partial win=25k, draw=10k, loss=0
	int32 CBillReward = 0;
	switch (Result)
	{
	case EMC2MissionResult::PlayerWinBig:    CBillReward = 50000; break;
	case EMC2MissionResult::PlayerWinSmall:  CBillReward = 25000; break;
	case EMC2MissionResult::Draw:            CBillReward = 10000; break;
	default: break;
	}
	CBillReward += PlayerKills * 5000;  // bonus per kill

	LS->RecordMissionResult(GetWorld()->GetMapName(), (uint8)Result, CBillReward);
	LS->AdvanceToNextMission();
	LS->SaveGame(0);
}

// ---------------------------------------------------------------------------
// ABL-equivalent script API
// ---------------------------------------------------------------------------

void AMC2GameMode::ScriptSetMissionFlag(int32 FlagIndex, bool Value)
{
	if (AMC2GameState* GS = GetGameState<AMC2GameState>())
		GS->SetMissionFlag(FlagIndex, Value);
}

bool AMC2GameMode::ScriptGetMissionFlag(int32 FlagIndex) const
{
	if (const AMC2GameState* GS = GetGameState<AMC2GameState>())
		return GS->GetMissionFlag(FlagIndex);
	return false;
}

void AMC2GameMode::ScriptStartTimer(int32 TimerIndex)
{
	if (AMC2GameState* GS = GetGameState<AMC2GameState>())
		GS->StartTimer(TimerIndex);
}

float AMC2GameMode::ScriptGetTimer(int32 TimerIndex) const
{
	if (const AMC2GameState* GS = GetGameState<AMC2GameState>())
		return GS->GetTimerValue(TimerIndex);
	return 0.f;
}

void AMC2GameMode::ScriptSetMissionResult(EMC2MissionResult Result)
{
	ApplyMissionResult(Result);
}

// ---------------------------------------------------------------------------
// Unit queries
// ---------------------------------------------------------------------------

TArray<AMC2Mover*> AMC2GameMode::GetUnitsByTeam(int32 TeamIndex) const
{
	TArray<AMC2Mover*> Result;
	for (AMC2Mover* Unit : AllSpawnedUnits)
		if (Unit && Unit->TeamIndex == TeamIndex)
			Result.Add(Unit);
	return Result;
}

bool AMC2GameMode::IsTeamDefeated(int32 TeamIndex) const
{
	for (AMC2Mover* Unit : AllSpawnedUnits)
		if (Unit && !Unit->bIsDestroyed && Unit->TeamIndex == TeamIndex)
			return false;
	return true;
}
