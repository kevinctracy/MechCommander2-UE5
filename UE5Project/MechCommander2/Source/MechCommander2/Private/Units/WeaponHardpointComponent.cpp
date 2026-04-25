#include "Units/WeaponHardpointComponent.h"
#include "Units/MC2Mover.h"
#include "Units/MC2HealthComponent.h"
#include "Combat/MC2Projectile.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/World.h"

UWeaponHardpointComponent::UWeaponHardpointComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UWeaponHardpointComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CooldownRemaining > 0.f)
		CooldownRemaining = FMath::Max(0.f, CooldownRemaining - DeltaTime);
}

bool UWeaponHardpointComponent::CanFire() const
{
	if (bDestroyed)
		return false;
	if (CooldownRemaining > 0.f)
		return false;
	if (MaxAmmo > 0 && CurrentAmmo <= 0)
		return false;

	// Check heat — if owner is shut down, can't fire
	AMC2Mover* Owner = Cast<AMC2Mover>(GetOwner());
	if (Owner && Owner->bShutDown)
		return false;

	return true;
}

bool UWeaponHardpointComponent::FireAt(const FVector& TargetLocation, AActor* TargetActor)
{
	if (!CanFire())
		return false;

	if (!GetOwner()->HasAuthority())
		return false;

	CooldownRemaining = Cooldown;

	if (MaxAmmo > 0)
		--CurrentAmmo;

	// Generate heat on owner
	AMC2Mover* Owner = Cast<AMC2Mover>(GetOwner());
	if (Owner)
		Owner->AddHeat(HeatGenerated);

	// Spawn muzzle flash
	if (MuzzleFlashEffect)
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), MuzzleFlashEffect, GetComponentLocation(), GetComponentRotation());

	// Energy weapons: hitscan. Ballistic/Missile: projectile actor.
	if (WeaponType == EMC2WeaponType::Energy)
		SpawnHitscan(TargetLocation, TargetActor);
	else
		SpawnProjectile(TargetLocation, TargetActor);

	return true;
}

void UWeaponHardpointComponent::SpawnHitscan(const FVector& TargetLocation, AActor* TargetActor)
{
	// Energy weapons hit instantly — find what's between us and target
	FVector Start = GetComponentLocation();
	FVector End   = TargetLocation;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params))
	{
		if (AMC2Mover* HitMover = Cast<AMC2Mover>(Hit.GetActor()))
		{
			if (UMC2HealthComponent* HC = HitMover->FindComponentByClass<UMC2HealthComponent>())
				HC->ApplyDamage(Damage, Hit.BoneName, GetOwner());
		}
	}
}

void UWeaponHardpointComponent::SpawnProjectile(const FVector& TargetLocation, AActor* TargetActor)
{
	if (!ProjectileClass)
		return;

	FVector SpawnLoc = GetComponentLocation();
	FRotator SpawnRot = (TargetLocation - SpawnLoc).Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner   = GetOwner();
	SpawnParams.Instigator = Cast<APawn>(GetOwner());

	AMC2Projectile* Proj = GetWorld()->SpawnActor<AMC2Projectile>(ProjectileClass, SpawnLoc, SpawnRot, SpawnParams);
	if (Proj)
	{
		Proj->Damage      = Damage;
		Proj->TargetActor = TargetActor;
		Proj->Launch(TargetLocation);
	}
}

void UWeaponHardpointComponent::DestroyWeapon()
{
	bDestroyed = true;
	CurrentAmmo = 0;
}
