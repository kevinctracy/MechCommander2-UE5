#include "Combat/MC2Projectile.h"
#include "Units/MC2Mover.h"
#include "Units/MC2HealthComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

AMC2Projectile::AMC2Projectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	CollisionSphere->InitSphereRadius(20.f);
	CollisionSphere->SetCollisionProfileName(TEXT("Projectile"));
	SetRootComponent(CollisionSphere);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	ProjectileMovement->InitialSpeed   = 3000.f;
	ProjectileMovement->MaxSpeed       = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.f;  // no gravity by default

	InitialLifeSpan = Lifetime;
}

void AMC2Projectile::BeginPlay()
{
	Super::BeginPlay();
	CollisionSphere->OnComponentHit.AddDynamic(this, &AMC2Projectile::OnHit);
	InitialLifeSpan = Lifetime;
}

void AMC2Projectile::Launch(const FVector& TargetLocation)
{
	FVector Dir = (TargetLocation - GetActorLocation()).GetSafeNormal();
	ProjectileMovement->Velocity = Dir * ProjectileMovement->InitialSpeed;

	if (bIsHoming && TargetActor.IsValid())
		ProjectileMovement->HomingTargetComponent = TargetActor->GetRootComponent();
}

void AMC2Projectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
                            UPrimitiveComponent* OtherComp, FVector NormalImpulse,
                            const FHitResult& Hit)
{
	if (!HasAuthority())
		return;

	ApplyImpactDamage(Hit);

	if (SplashRadius > 0.f)
		ApplySplashDamage(GetActorLocation());

	if (ImpactEffect)
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactEffect, GetActorLocation(), Hit.ImpactNormal.Rotation());

	BP_OnImpact(Hit);
	Destroy();
}

void AMC2Projectile::ApplyImpactDamage(const FHitResult& Hit)
{
	AMC2Mover* HitMover = Cast<AMC2Mover>(Hit.GetActor());
	if (!HitMover)
		return;

	if (UMC2HealthComponent* HC = HitMover->FindComponentByClass<UMC2HealthComponent>())
		HC->ApplyDamage(Damage, Hit.BoneName, GetInstigator());
}

void AMC2Projectile::ApplySplashDamage(const FVector& Location)
{
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(GetOwner());

	GetWorld()->OverlapMultiByObjectType(
		Overlaps, Location, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(SplashRadius),
		Params
	);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AMC2Mover* Mover = Cast<AMC2Mover>(Overlap.GetActor());
		if (!Mover)
			continue;

		float Dist  = FVector::Dist(Location, Mover->GetActorLocation());
		float Scale = FMath::Clamp(1.f - Dist / SplashRadius, 0.f, 1.f);

		if (UMC2HealthComponent* HC = Mover->FindComponentByClass<UMC2HealthComponent>())
			HC->ApplyDamage(Damage * Scale, NAME_None, GetInstigator());
	}
}

void AMC2Projectile::Intercept()
{
	if (ImpactEffect)
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactEffect, GetActorLocation());
	Destroy();
}

// ---------------------------------------------------------------------------
// Missile
// ---------------------------------------------------------------------------

AMC2Missile::AMC2Missile()
{
	PrimaryActorTick.bCanEverTick = true;
	bIsHoming = true;
	ProjectileMovement->InitialSpeed  = 1200.f;
	ProjectileMovement->MaxSpeed      = 1800.f;
	ProjectileMovement->bIsHomingProjectile = true;
	ProjectileMovement->HomingAccelerationMagnitude = HomingAcceleration;
}

void AMC2Missile::BeginPlay()
{
	Super::BeginPlay();
}

void AMC2Missile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TimeAlive += DeltaSeconds;

	// Activate homing after arm time
	if (TimeAlive >= ArmTime && TargetActor.IsValid())
	{
		if (!ProjectileMovement->HomingTargetComponent.IsValid())
			ProjectileMovement->HomingTargetComponent = TargetActor->GetRootComponent();
	}
}
