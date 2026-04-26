#include "Units/MC2GroundVehicle.h"
#include "Net/UnrealNetwork.h"
#include "Components/SkeletalMeshComponent.h"

AMC2GroundVehicle::AMC2GroundVehicle()
{
	PrimaryActorTick.bCanEverTick = true;
	InitDefaultArmor();
}

void AMC2GroundVehicle::InitDefaultArmor()
{
	VehicleArmor.Empty();
	for (uint8 i = 0; i < 5; ++i)
	{
		FMC2VehicleArmor Zone;
		Zone.Arc = (EMC2VehicleArc)i;
		VehicleArmor.Add(Zone);
	}
}

void AMC2GroundVehicle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMC2GroundVehicle, VehicleArmor);
	DOREPLIFETIME(AMC2GroundVehicle, CurrentTurretYaw);
}

// ---------------------------------------------------------------------------
// Armor
// ---------------------------------------------------------------------------

bool AMC2GroundVehicle::ApplyVehicleDamage(float Damage, EMC2VehicleArc HitArc)
{
	for (FMC2VehicleArmor& Zone : VehicleArmor)
	{
		if (Zone.Arc != HitArc) continue;

		// Armor absorbs first
		float ArmorAbsorb = FMath::Min(Damage, Zone.CurrentArmor);
		Zone.CurrentArmor -= ArmorAbsorb;
		Damage            -= ArmorAbsorb;

		// Remaining damage hits internal structure
		if (Damage > 0.f)
		{
			Zone.CurrentInternalStructure -= Damage;
			if (Zone.CurrentInternalStructure <= 0.f)
			{
				// Any arc IS reaching 0 destroys the vehicle
				Zone.CurrentInternalStructure = 0.f;
				OnDestroyed_MC2();
				return true;
			}
		}
		return false;
	}
	return false;
}

float AMC2GroundVehicle::GetArcArmor(EMC2VehicleArc Arc) const
{
	for (const FMC2VehicleArmor& Zone : VehicleArmor)
		if (Zone.Arc == Arc) return Zone.CurrentArmor;
	return 0.f;
}

void AMC2GroundVehicle::OnRep_VehicleArmor()
{
	// HUD refresh hook (BP can bind to this via RepNotify)
}

EMC2VehicleArc AMC2GroundVehicle::GetHitArcFromDirection(const FVector& DamageOrigin) const
{
	const FVector ToDamage = (DamageOrigin - GetActorLocation()).GetSafeNormal2D();
	const FVector Forward  = GetActorForwardVector().GetSafeNormal2D();
	const FVector Right    = GetActorRightVector().GetSafeNormal2D();

	const float Fwd   = FVector::DotProduct(ToDamage, Forward);
	const float Right_ = FVector::DotProduct(ToDamage, Right);

	// Front/rear arc: ±45° from forward/backward
	if (Fwd >= 0.707f)  return EMC2VehicleArc::Front;
	if (Fwd <= -0.707f) return EMC2VehicleArc::Rear;
	return (Right_ > 0.f) ? EMC2VehicleArc::Right : EMC2VehicleArc::Left;
}

// ---------------------------------------------------------------------------
// Turret
// ---------------------------------------------------------------------------

void AMC2GroundVehicle::AimTurretAt(const FVector& WorldTarget)
{
	const FVector ToTarget = (WorldTarget - GetActorLocation()).GetSafeNormal();
	const FRotator DesiredWorld = ToTarget.Rotation();

	// Convert to vehicle-local yaw
	const float VehicleYaw = GetActorRotation().Yaw;
	float RelativeYaw = DesiredWorld.Yaw - VehicleYaw;

	// Normalize to [-180, 180] and clamp to turret limits
	while (RelativeYaw > 180.f)  RelativeYaw -= 360.f;
	while (RelativeYaw < -180.f) RelativeYaw += 360.f;

	const float HalfLimit = MaxTurretYaw * 0.5f;
	TargetTurretRotation.Yaw = FMath::Clamp(RelativeYaw, -HalfLimit, HalfLimit);
}

void AMC2GroundVehicle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Interpolate turret toward target yaw
	const float MaxDelta = TurretYawRate * DeltaSeconds;
	CurrentTurretYaw = FMath::FixedTurn(CurrentTurretYaw, TargetTurretRotation.Yaw, MaxDelta);

	// Apply to turret bone on skeletal mesh
	if (USkeletalMeshComponent* SMC = FindComponentByClass<USkeletalMeshComponent>())
	{
		if (SMC->DoesSocketExist(TurretBone))
		{
			// Turret bone rotation is set relative to vehicle — use SetBoneRotationByName
			SMC->SetBoneRotationByName(TurretBone,
				FRotator(0.f, CurrentTurretYaw, 0.f),
				EBoneSpaces::ComponentSpace);
		}
	}
}
