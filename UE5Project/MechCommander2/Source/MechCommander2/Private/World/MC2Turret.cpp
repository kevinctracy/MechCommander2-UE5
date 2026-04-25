#include "World/MC2Turret.h"
#include "Units/SensorComponent.h"
#include "Units/WeaponHardpointComponent.h"
#include "Components/StaticMeshComponent.h"

AMC2Turret::AMC2Turret()
{
	PrimaryActorTick.bCanEverTick = true;

	BaseMesh   = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	TurretMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretMesh"));

	SetRootComponent(BaseMesh);
	TurretMesh->SetupAttachment(BaseMesh);
	BaseMesh->SetCollisionProfileName(TEXT("BlockAll"));
	TurretMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// Turrets don't move
	MaxMoveSpeed = 0.f;
}

void AMC2Turret::BeginPlay()
{
	Super::BeginPlay();
}

void AMC2Turret::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsDestroyed) return;

	AimTurretAtTarget(DeltaSeconds);

	if (bAutoFire)
		FireIfReady();
}

void AMC2Turret::AimTurretAtTarget(float DeltaSeconds)
{
	// Pick nearest enemy from sensor
	USensorComponent* Sensor = FindComponentByClass<USensorComponent>();
	if (!Sensor) return;

	AMC2Mover* NearestEnemy = Sensor->GetNearestEnemy();
	CurrentTarget = NearestEnemy;

	if (!NearestEnemy) return;

	FVector ToTarget = (NearestEnemy->GetActorLocation() - TurretMesh->GetComponentLocation()).GetSafeNormal();
	FRotator DesiredRot = ToTarget.Rotation();

	// Only yaw the turret mesh; clamp if MaxTurretYaw > 0
	float TargetYaw = DesiredRot.Yaw - GetActorRotation().Yaw;
	if (MaxTurretYaw > 0.f)
		TargetYaw = FMath::ClampAngle(TargetYaw, -MaxTurretYaw, MaxTurretYaw);

	float MaxDelta = TurretYawRate * DeltaSeconds;
	CurrentTurretYaw = FMath::FixedTurn(CurrentTurretYaw, TargetYaw, MaxDelta);

	TurretMesh->SetRelativeRotation(FRotator(0.f, CurrentTurretYaw, 0.f));
}

void AMC2Turret::FireIfReady()
{
	AMC2Mover* Target = CurrentTarget.Get();
	if (!Target || Target->bIsDestroyed) return;

	for (UWeaponHardpointComponent* WC : WeaponHardpoints)
	{
		if (!WC || WC->bDestroyed || WC->CooldownRemaining > 0.f) continue;
		float DistSq = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
		if (DistSq <= WC->WeaponRange * WC->WeaponRange)
			WC->FireAt(Target);
	}
}
