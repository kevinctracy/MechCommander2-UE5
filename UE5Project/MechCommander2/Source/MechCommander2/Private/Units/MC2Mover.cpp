#include "Units/MC2Mover.h"
#include "Units/MC2HealthComponent.h"
#include "Units/SensorComponent.h"
#include "Units/WeaponHardpointComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "NavigationInvokerComponent.h"
#include "Net/UnrealNetwork.h"
#include "AIController.h"
#include "MC2GameState.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraFunctionLibrary.h"

AMC2Mover::AMC2Mover()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	MovementComponent->MaxSpeed = MaxMoveSpeed;

	HealthComponent = CreateDefaultSubobject<UMC2HealthComponent>(TEXT("Health"));

	SensorComponent = CreateDefaultSubobject<USensorComponent>(TEXT("Sensor"));

	NavInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));
}

void AMC2Mover::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMC2Mover, TeamIndex);
	DOREPLIFETIME(AMC2Mover, ArmorZones);
	DOREPLIFETIME(AMC2Mover, CurrentHeat);
	DOREPLIFETIME(AMC2Mover, bShutDown);
	DOREPLIFETIME(AMC2Mover, bLegsDamaged);
	DOREPLIFETIME(AMC2Mover, bImmobilized);
	DOREPLIFETIME(AMC2Mover, bIsDestroyed);
}

void AMC2Mover::BeginPlay()
{
	Super::BeginPlay();

	// Sync movement speed to data
	if (MovementComponent)
		MovementComponent->MaxSpeed = MaxMoveSpeed;

	// Register with game state
	if (AMC2GameState* GS = GetWorld()->GetGameState<AMC2GameState>())
		GS->RegisterTeam(TeamIndex, FString::Printf(TEXT("Team_%d"), TeamIndex));
}

void AMC2Mover::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
		return;

	UpdateHeat(DeltaSeconds);
}

void AMC2Mover::AddHeat(float Amount)
{
	if (!HasAuthority())
		return;
	CurrentHeat = FMath::Min(CurrentHeat + Amount, MaxHeat);
	UpdateShutdown();
}

void AMC2Mover::UpdateHeat(float DeltaSeconds)
{
	if (CurrentHeat <= 0.f)
		return;

	// Count heatsinks for bonus dissipation
	float TotalDissipation = HeatDissipationPerSecond;
	for (UWeaponHardpointComponent* HP : WeaponHardpoints)
	{
		if (IsValid(HP))
			TotalDissipation += HP->GetHeatSinkBonus();
	}

	CurrentHeat = FMath::Max(0.f, CurrentHeat - TotalDissipation * DeltaSeconds);
	UpdateShutdown();
}

void AMC2Mover::UpdateShutdown()
{
	bool bShouldShutDown = CurrentHeat >= OverheatShutdownThreshold;
	if (bShouldShutDown != bShutDown)
	{
		bShutDown = bShouldShutDown;
		if (MovementComponent)
			MovementComponent->MaxSpeed = bShutDown ? 0.f : (bLegsDamaged ? MaxMoveSpeed * LimpSpeedMultiplier : MaxMoveSpeed);
	}
}

void AMC2Mover::SetSelected(bool bSelected)
{
	bIsSelected = bSelected;
	OnSelectionChanged(bSelected);
}

void AMC2Mover::ReceiveMoveOrder(const FVector& Destination)
{
	CurrentOrder.OrderType      = EMC2OrderType::Move;
	CurrentOrder.TargetLocation = Destination;
	CurrentOrder.TargetActor    = nullptr;

	if (AAIController* AIC = Cast<AAIController>(GetController()))
		AIC->MoveToLocation(Destination, 50.f);
}

void AMC2Mover::ReceiveAttackOrder(AActor* Target)
{
	CurrentOrder.OrderType   = EMC2OrderType::Attack;
	CurrentOrder.TargetActor = Target;

	if (AAIController* AIC = Cast<AAIController>(GetController()))
		AIC->MoveToActor(Target, 100.f);
}

void AMC2Mover::ReceiveGuardOrder(const FVector& Position)
{
	CurrentOrder.OrderType      = EMC2OrderType::Guard;
	CurrentOrder.TargetLocation = Position;
	CurrentOrder.TargetActor    = nullptr;

	if (AAIController* AIC = Cast<AAIController>(GetController()))
		AIC->MoveToLocation(Position, 50.f);
}

void AMC2Mover::OnDestroyed_MC2()
{
	if (bIsDestroyed)
		return;
	bIsDestroyed = true;
	MaxMoveSpeed = 0.f;

	// Notify game state
	if (AMC2GameState* GS = GetWorld()->GetGameState<AMC2GameState>())
		GS->OnUnitDestroyed(TeamIndex);

	// Detach AI and disable input
	if (AAIController* AIC = Cast<AAIController>(GetController()))
		AIC->StopMovement();
	DisableInput(nullptr);

	// Spawn explosion Niagara effect (server-side; will replicate via actor)
	if (HasAuthority() && ExplosionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ExplosionEffect,
			GetActorLocation(),
			FRotator::ZeroRotator,
			FVector(ExplosionScale),
			/*bAutoDestroy=*/ true
		);
	}

	// Swap skeletal mesh to destroyed static mesh variant
	if (DestroyedMesh)
	{
		// Hide the skeletal mesh
		if (USkeletalMeshComponent* SMC = FindComponentByClass<USkeletalMeshComponent>())
			SMC->SetVisibility(false);

		// Spawn a static mesh component for the wreck
		UStaticMeshComponent* WreckComp = NewObject<UStaticMeshComponent>(this, TEXT("WreckMesh"));
		WreckComp->SetStaticMesh(DestroyedMesh);
		WreckComp->SetWorldTransform(GetActorTransform());
		WreckComp->RegisterComponent();
		WreckComp->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
	}

	OnMoverDestroyed.Broadcast(this);
	BP_OnDestroyed();
}

void AMC2Mover::OnRep_ArmorZones()
{
	// HUD can bind to this to refresh the armor display
}
