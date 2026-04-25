#include "Units/MC2PilotComponent.h"
#include "Units/MC2Mover.h"
#include "Campaign/MC2LogisticsSubsystem.h"
#include "Kismet/KismetMathLibrary.h"

UMC2PilotComponent::UMC2PilotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMC2PilotComponent::BeginPlay()
{
	Super::BeginPlay();
}

int32 UMC2PilotComponent::GetEffectiveGunnery() const
{
	// Each injury level raises Gunnery by 1 (worse aim)
	return FMath::Min(Gunnery + InjuryLevel, 12);
}

int32 UMC2PilotComponent::GetEffectivePiloting() const
{
	return FMath::Min(Piloting + InjuryLevel, 12);
}

float UMC2PilotComponent::GetBaseHitChance() const
{
	// BattleTech 2d6 target number: Gunnery+2 to hit on 2d6
	// P(2d6 >= N) lookup: 2=1.0, 3=0.972, 4=0.917, 5=0.833, 6=0.722, 7=0.583, 8=0.417, 9=0.278, 10=0.167, 11=0.083, 12=0.028
	static const float HitTable[] = { 1.f, 1.f, 1.f, 0.972f, 0.917f, 0.833f, 0.722f, 0.583f, 0.417f, 0.278f, 0.167f, 0.083f, 0.028f };
	int32 TargetNumber = GetEffectiveGunnery() + 2;
	TargetNumber = FMath::Clamp(TargetNumber, 2, 12);
	return HitTable[TargetNumber];
}

void UMC2PilotComponent::TakeInjury(int32 Severity)
{
	if (bIsKIA) return;

	InjuryLevel += Severity;

	if (InjuryLevel >= 4)
	{
		InjuryLevel = 4;
		bIsKIA = true;
		OnPilotKilled.Broadcast(PilotID);
	}
}

void UMC2PilotComponent::RecordKill()
{
	MissionKills++;
	MissionXP += 150;  // base XP per kill; bonuses applied at mission end
}

bool UMC2PilotComponent::RollPilotingCheck(int32 Modifier)
{
	// Roll 2d6 vs (EffectivePiloting + 3 + Modifier)
	int32 Roll = FMath::RandRange(1, 6) + FMath::RandRange(1, 6);
	int32 Target = GetEffectivePiloting() + 3 + Modifier;
	return Roll >= Target;
}

void UMC2PilotComponent::CommitXPToSave()
{
	if (PilotID.IsNone()) return;

	UGameInstance* GI = GetOwner() ? GetOwner()->GetGameInstance() : nullptr;
	if (!GI) return;

	UMC2LogisticsSubsystem* LS = GI->GetSubsystem<UMC2LogisticsSubsystem>();
	if (!LS) return;

	LS->AwardPilotXP(PilotID, MissionXP);

	if (bIsKIA)
		LS->KillPilot(PilotID);

	MissionXP   = 0;
	MissionKills = 0;
}
