#include "Units/MC2BattleMech.h"
#include "MC2DataTableRows.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "FX/MC2FXLibrary.h"

AMC2BattleMech::AMC2BattleMech()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMC2BattleMech::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMC2BattleMech, TorsoYawOffset);
	DOREPLIFETIME(AMC2BattleMech, bIsJumping);
}

// ---------------------------------------------------------------------------
// Torso twist
// ---------------------------------------------------------------------------

void AMC2BattleMech::SetTorsoAimDirection(const FVector& WorldDirection)
{
	if (WorldDirection.IsNearlyZero()) return;

	// Convert world aim direction to a yaw relative to the mech's leg direction
	const float WorldYaw = WorldDirection.Rotation().Yaw;
	const float LegYaw   = GetActorRotation().Yaw;
	float Relative = WorldYaw - LegYaw;

	// Normalize to [-180, 180]
	while (Relative >  180.f) Relative -= 360.f;
	while (Relative < -180.f) Relative += 360.f;

	DesiredTorsoYaw = FMath::Clamp(Relative, -MaxTorsoYaw, MaxTorsoYaw);
}

void AMC2BattleMech::AimTorsoAt(const FVector& WorldLocation)
{
	const FVector Dir = (WorldLocation - GetActorLocation()).GetSafeNormal();
	SetTorsoAimDirection(Dir);
}

// ---------------------------------------------------------------------------
// Tick — interpolate torso and advance jump arc
// ---------------------------------------------------------------------------

void AMC2BattleMech::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Torso: interpolate toward desired yaw at TurretYawRate
	const float MaxDelta = TorsoYawRate * DeltaSeconds;
	TorsoYawOffset = FMath::FixedTurn(TorsoYawOffset, DesiredTorsoYaw, MaxDelta);

	// If torso has walked back to center and legs are facing target, allow full turn
	// (When TorsoYawOffset hits ±MaxTorsoYaw the mech body rotates to keep up — handled in ABP)

	// Jump arc
	if (bIsJumping && HasAuthority())
	{
		JumpAlpha += DeltaSeconds / JumpDuration;
		if (JumpAlpha >= 1.f)
		{
			JumpAlpha = 1.f;
			SetActorLocation(JumpTarget);
			LandFromJump();
		}
		else
		{
			// Parabolic arc: linear XY + sine Z
			const FVector XY  = FMath::Lerp(FVector(JumpOrigin.X, JumpOrigin.Y, 0.f),
			                                FVector(JumpTarget.X,  JumpTarget.Y,  0.f),
			                                JumpAlpha);
			const float ArcH  = FMath::Sin(JumpAlpha * PI) * JumpRange * 0.25f;
			const float BaseZ = FMath::Lerp(JumpOrigin.Z, JumpTarget.Z, JumpAlpha);
			SetActorLocation(FVector(XY.X, XY.Y, BaseZ + ArcH));
		}
	}
}

// ---------------------------------------------------------------------------
// Jump Jets
// ---------------------------------------------------------------------------

void AMC2BattleMech::StartJump(const FVector& TargetLocation)
{
	if (!bHasJumpJets || bIsJumping || bIsDestroyed) return;

	JumpOrigin   = GetActorLocation();
	JumpTarget   = TargetLocation;
	JumpAlpha    = 0.f;
	bIsJumping   = true;

	// Spawn jet exhaust FX on both ankle sockets (FX loops until LandFromJump)
	UMC2FXLibrary::SpawnFXAttached(EMC2FXType::JumpJetExhaust,
		GetRootComponent(), TEXT("ankle_l"));
	UMC2FXLibrary::SpawnFXAttached(EMC2FXType::JumpJetExhaust,
		GetRootComponent(), TEXT("ankle_r"));
}

void AMC2BattleMech::LandFromJump()
{
	bIsJumping = false;

	// Spawn landing dust cloud
	UMC2FXLibrary::SpawnFX(this, EMC2FXType::DustCloud, GetActorLocation(),
		FRotator::ZeroRotator, 2.f);
}

// ---------------------------------------------------------------------------
// Chassis data init
// ---------------------------------------------------------------------------

void AMC2BattleMech::InitFromChassisID(FName ChassisID, UDataTable* ChassisTable)
{
	if (!ChassisTable) return;
	FMC2MechChassisRow* Row = ChassisTable->FindRow<FMC2MechChassisRow>(ChassisID, TEXT("InitMech"));
	if (!Row) return;

	MaxTorsoYaw  = (float)Row->MaxTorsoYaw;
	TorsoYawRate = (float)Row->TorsoYawRate;
	MaxMoveSpeed = Row->MaxRunSpeed * 27.78f;  // km/h → cm/s (UE units)
}
