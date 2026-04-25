#include "Units/MC2HealthComponent.h"
#include "Units/MC2Mover.h"
#include "Units/MC2PilotComponent.h"
#include "Units/WeaponHardpointComponent.h"
#include "Kismet/GameplayStatics.h"

UMC2HealthComponent::UMC2HealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMC2HealthComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UMC2HealthComponent::ApplyDamage(float Damage, FName HitBone, AActor* Instigator)
{
	if (bDestroyed || Damage <= 0.f)
		return;

	AMC2Mover* Owner = Cast<AMC2Mover>(GetOwner());
	if (!Owner)
		return;

	// Find the right armor zone from bone name
	FMC2ArmorZone* Zone = FindZoneForBone(HitBone);
	if (!Zone && Owner->ArmorZones.Num() > 0)
		Zone = &Owner->ArmorZones[0];   // fallback to first zone

	if (Zone)
		ApplyDamageToZoneData(*Zone, Damage, Instigator);

	CheckDestroyed();
}

void UMC2HealthComponent::ApplyDamageToZone(float Damage, EMC2HitArc Arc, EMC2HitSection Section, AActor* Instigator)
{
	AMC2Mover* Owner = Cast<AMC2Mover>(GetOwner());
	if (!Owner || bDestroyed)
		return;

	for (FMC2ArmorZone& Zone : Owner->ArmorZones)
	{
		if (Zone.Arc == Arc && Zone.Section == Section)
		{
			ApplyDamageToZoneData(Zone, Damage, Instigator);
			break;
		}
	}
	CheckDestroyed();
}

void UMC2HealthComponent::ApplyDamageToZoneData(FMC2ArmorZone& Zone, float Damage, AActor* Instigator)
{
	AMC2Mover* Owner = Cast<AMC2Mover>(GetOwner());

	// Apply to armor first
	float ArmorAbsorb = FMath::Min(Zone.CurrentArmor, Damage);
	Zone.CurrentArmor -= ArmorAbsorb;
	float Overflow     = Damage - ArmorAbsorb;

	OnArmorDamaged.Broadcast(Owner, Zone.BoneName, Damage, Zone.CurrentArmor);

	// Overflow damage goes to internal structure
	if (Overflow > 0.f && Zone.CurrentInternalStructure > 0.f)
	{
		float ISAbsorb = FMath::Min(Zone.CurrentInternalStructure, Overflow);
		Zone.CurrentInternalStructure -= ISAbsorb;

		if (Zone.CurrentInternalStructure <= 0.f)
		{
			// Section destroyed — roll for critical component hit
			TriggerCriticalHit(Zone);
			OnSectionDestroyed.Broadcast(Owner, Zone.BoneName);

			// Special cases for mech legs
			FString BoneStr = Zone.BoneName.ToString().ToLower();
			if (BoneStr.Contains("leg"))
				Owner->bLegsDamaged = true;
		}
	}
}

void UMC2HealthComponent::TriggerCriticalHit(const FMC2ArmorZone& Zone)
{
	if (FMath::FRand() < CriticalHitChance)
		ApplyCritConsequences(Zone);
}

void UMC2HealthComponent::ApplyCritConsequences(const FMC2ArmorZone& Zone)
{
	AMC2Mover* Owner = Cast<AMC2Mover>(GetOwner());
	if (!Owner)
		return;

	FString BoneStr = Zone.BoneName.ToString().ToLower();
	bool bIsTorso   = BoneStr.Contains("torso") || BoneStr.Contains("chest") || BoneStr.Contains("ct");
	bool bIsLeg     = BoneStr.Contains("leg") || BoneStr.Contains("ankle");

	// --- Engine crit (center torso / torso sections) ---
	if (bIsTorso)
	{
		EngineCritCount++;
		OnEngineCrit(EngineCritCount);

		// 1 engine crit: -25% max speed; 2+ crits: engine destroyed → shutdown
		if (EngineCritCount >= 2)
		{
			Owner->MaxMoveSpeed = 0.f;
			Owner->bShutDown = true;
			Owner->bImmobilized = true;
		}
		else
		{
			Owner->MaxMoveSpeed *= 0.75f;
		}
	}

	// --- Gyro crit (center torso) ---
	if (BoneStr.Contains("ct") || BoneStr.Contains("center"))
	{
		GyroCritCount++;
		OnGyroCrit(GyroCritCount);

		// Apply piloting check when gyro is hit
		if (UMC2PilotComponent* Pilot = Owner->FindComponentByClass<UMC2PilotComponent>())
		{
			const int32 GyroPilotingMod = GyroCritCount;  // +1 per crit
			Pilot->RollPilotingCheck(GyroPilotingMod);    // may apply injury if fail
		}
	}

	// --- Weapon crit (roll against each hardpoint in this section) ---
	TArray<UWeaponHardpointComponent*> Hardpoints;
	Owner->GetComponents<UWeaponHardpointComponent>(Hardpoints);
	for (int32 i = 0; i < Hardpoints.Num(); ++i)
	{
		UWeaponHardpointComponent* WC = Hardpoints[i];
		if (!WC || WC->bDestroyed)
			continue;

		// Only hit weapons in matching body section
		if (!WC->GetName().ToLower().Contains(BoneStr.Left(2)))
			continue;

		if (FMath::FRand() < CriticalHitChance)
		{
			WC->bDestroyed = true;
			OnWeaponCrit(i);
		}
	}

	// --- Leg actuator crit ---
	if (bIsLeg)
	{
		Owner->bLegsDamaged = true;
		Owner->MaxMoveSpeed *= Owner->LimpSpeedMultiplier;
		OnActuatorCrit(Zone.BoneName);
	}

	// --- Pilot injury from crit hit (cockpit / head) ---
	if (BoneStr.Contains("head") || BoneStr.Contains("cockpit"))
	{
		if (UMC2PilotComponent* Pilot = Owner->FindComponentByClass<UMC2PilotComponent>())
			Pilot->TakeInjury(1);
	}
}

void UMC2HealthComponent::CheckDestroyed()
{
	AMC2Mover* Owner = Cast<AMC2Mover>(GetOwner());
	if (!Owner || bDestroyed)
		return;

	// Unit is destroyed when all internal structure is gone
	bool bAllGone = true;
	for (const FMC2ArmorZone& Zone : Owner->ArmorZones)
	{
		if (Zone.CurrentInternalStructure > 0.f)
		{
			bAllGone = false;
			break;
		}
	}

	if (bAllGone)
	{
		bDestroyed = true;
		OnMoverKilled.Broadcast(Owner);
		Owner->OnDestroyed_MC2();
	}
}

float UMC2HealthComponent::GetTotalArmorPercent() const
{
	AMC2Mover* Owner = Cast<AMC2Mover>(GetOwner());
	if (!Owner || Owner->ArmorZones.IsEmpty())
		return 0.f;

	float Total = 0.f, Max = 0.f;
	for (const FMC2ArmorZone& Zone : Owner->ArmorZones)
	{
		Total += Zone.CurrentArmor + Zone.CurrentInternalStructure;
		Max   += Zone.MaxArmor     + Zone.MaxInternalStructure;
	}
	return Max > 0.f ? Total / Max : 0.f;
}

FMC2ArmorZone* UMC2HealthComponent::FindZoneForBone(FName BoneName)
{
	AMC2Mover* Owner = Cast<AMC2Mover>(GetOwner());
	if (!Owner)
		return nullptr;

	// Direct bone name match first
	for (FMC2ArmorZone& Zone : Owner->ArmorZones)
	{
		if (Zone.BoneName == BoneName)
			return &Zone;
	}

	// Heuristic fallback: derive arc and section from bone name string
	EMC2HitArc    Arc     = ArcFromBoneName(BoneName);
	EMC2HitSection Section = SectionFromBoneName(BoneName);

	for (FMC2ArmorZone& Zone : Owner->ArmorZones)
	{
		if (Zone.Arc == Arc && Zone.Section == Section)
			return &Zone;
	}

	return nullptr;
}

EMC2HitArc UMC2HealthComponent::ArcFromBoneName(FName BoneName) const
{
	FString S = BoneName.ToString().ToLower();
	if (S.Contains("rear") || S.Contains("back")) return EMC2HitArc::Rear;
	if (S.Contains("left") || S.Contains("_l_"))  return EMC2HitArc::Left;
	if (S.Contains("right")|| S.Contains("_r_"))  return EMC2HitArc::Right;
	return EMC2HitArc::Front;
}

EMC2HitSection UMC2HealthComponent::SectionFromBoneName(FName BoneName) const
{
	FString S = BoneName.ToString().ToLower();
	if (S.Contains("leg") || S.Contains("foot") || S.Contains("ankle")) return EMC2HitSection::Bottom;
	if (S.Contains("torso") || S.Contains("chest") || S.Contains("arm"))return EMC2HitSection::Middle;
	if (S.Contains("head") || S.Contains("cockpit"))                    return EMC2HitSection::Top;
	return EMC2HitSection::Middle;
}
