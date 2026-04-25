#include "World/MC2Building.h"
#include "Units/MC2HealthComponent.h"
#include "Combat/MC2Carnage.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

AMC2Building::AMC2Building()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));

	HealthComponent = CreateDefaultSubobject<UMC2HealthComponent>(TEXT("Health"));
}

void AMC2Building::BeginPlay()
{
	Super::BeginPlay();

	// Wire up the health component's depletion event
	// UMC2HealthComponent fires OnHealthDepleted when IS reaches 0
	// We bind via damage handling in the health component calling back to us
}

void AMC2Building::OnHealthDepleted(AActor* Instigator)
{
	DestroyBuilding(Instigator);
}

void AMC2Building::DestroyBuilding(AActor* Instigator)
{
	if (bIsDestroyed) return;
	bIsDestroyed = true;

	// Swap to destroyed mesh
	if (DestroyedMesh)
		Mesh->SetStaticMesh(DestroyedMesh);

	// Disable collision on destroyed building
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Spawn explosion
	if (ExplosionCarnageClass)
		AMC2Carnage::SpawnCarnage(this, ExplosionCarnageClass, GetActorLocation(), EMC2CarnageType::Explosion_Large);

	// Splash damage to nearby units
	if (ExplosionRadius > 0.f && ExplosionDamage > 0.f)
	{
		TArray<AActor*> Ignored;
		Ignored.Add(this);
		UGameplayStatics::ApplyRadialDamage(
			this, ExplosionDamage, GetActorLocation(),
			ExplosionRadius, UDamageType::StaticClass(),
			Ignored, Instigator, nullptr, true
		);
	}

	OnBuildingDestroyed.Broadcast(this);
}
