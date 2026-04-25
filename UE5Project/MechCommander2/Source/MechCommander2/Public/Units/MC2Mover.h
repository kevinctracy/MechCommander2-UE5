#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MC2Mover.generated.h"

class UMC2HealthComponent;
class USensorComponent;
class UWeaponHardpointComponent;
class UFloatingPawnMovement;
class UNavigationInvokerComponent;

// ---------------------------------------------------------------------------
// Armor zone data — matches MC2's NUM_MECH_HIT_ARCS x NUM_MECH_HIT_SECTIONS
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EMC2HitArc : uint8
{
	Front  UMETA(DisplayName = "Front"),
	Rear   UMETA(DisplayName = "Rear"),
	Left   UMETA(DisplayName = "Left"),
	Right  UMETA(DisplayName = "Right"),
};

UENUM(BlueprintType)
enum class EMC2HitSection : uint8
{
	Top    UMETA(DisplayName = "Top"),
	Middle UMETA(DisplayName = "Middle"),
	Bottom UMETA(DisplayName = "Bottom"),
};

USTRUCT(BlueprintType)
struct FMC2ArmorZone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BoneName;               // Skeleton bone that maps to this zone

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMC2HitArc Arc = EMC2HitArc::Front;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMC2HitSection Section = EMC2HitSection::Middle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxArmor = 10.f;

	UPROPERTY(BlueprintReadOnly)
	float CurrentArmor = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxInternalStructure = 5.f;

	UPROPERTY(BlueprintReadOnly)
	float CurrentInternalStructure = 5.f;
};

// ---------------------------------------------------------------------------
// Mover order types
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EMC2OrderType : uint8
{
	None,
	Move,
	Attack,
	AttackMove,
	Guard,
	Patrol,
};

USTRUCT(BlueprintType)
struct FMC2MoverOrder
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EMC2OrderType OrderType = EMC2OrderType::None;

	UPROPERTY(BlueprintReadOnly)
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<AActor> TargetActor;
};

// ---------------------------------------------------------------------------
// AMC2Mover
// Base class for all moveable combat units: BattleMechs, GroundVehicles, Turrets.
// Directly mirrors the Mover class hierarchy in mover.h/mover.cpp.
// ---------------------------------------------------------------------------

UCLASS(Abstract)
class MECHCOMMANDER2_API AMC2Mover : public APawn
{
	GENERATED_BODY()

public:
	AMC2Mover();

	// --- Team ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Team")
	int32 TeamIndex = 0;

	// --- Armor Zones (replicated for HUD + objectives) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_ArmorZones, Category = "Combat|Armor")
	TArray<FMC2ArmorZone> ArmorZones;

	UFUNCTION()
	void OnRep_ArmorZones();

	// --- Heat System ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Heat")
	float MaxHeat = 100.f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Combat|Heat")
	float CurrentHeat = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Heat")
	float HeatDissipationPerSecond = 5.f;     // base dissipation; heatsinks add to this

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Heat")
	float OverheatShutdownThreshold = 95.f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Combat|Heat")
	bool bShutDown = false;

	UFUNCTION(BlueprintCallable, Category = "Combat|Heat")
	void AddHeat(float Amount);

	// --- Movement ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	TObjectPtr<UFloatingPawnMovement> MovementComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	float MaxMoveSpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	float LimpSpeedMultiplier = 0.5f;         // applied when a leg is damaged

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Movement")
	bool bLegsDamaged = false;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Movement")
	bool bImmobilized = false;

	// --- Selection ---

	UPROPERTY(BlueprintReadOnly, Category = "Selection")
	bool bIsSelected = false;

	UFUNCTION(BlueprintCallable, Category = "Selection")
	void SetSelected(bool bSelected);

	UFUNCTION(BlueprintImplementableEvent, Category = "Selection")
	void OnSelectionChanged(bool bSelected);

	// --- Orders (called by PlayerController) ---

	UFUNCTION(BlueprintCallable, Category = "Orders")
	void ReceiveMoveOrder(const FVector& Destination);

	UFUNCTION(BlueprintCallable, Category = "Orders")
	void ReceiveAttackOrder(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Orders")
	void ReceiveGuardOrder(const FVector& Position);

	UFUNCTION(BlueprintReadOnly, Category = "Orders")
	const FMC2MoverOrder& GetCurrentOrder() const { return CurrentOrder; }

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UMC2HealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<USensorComponent> SensorComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	TObjectPtr<UNavigationInvokerComponent> NavInvoker;

	// Weapon hardpoints — configured per-unit in Blueprint
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TArray<TObjectPtr<UWeaponHardpointComponent>> WeaponHardpoints;

	// --- State ---

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State")
	bool bIsDestroyed = false;

	UFUNCTION(BlueprintCallable, Category = "State")
	virtual void OnDestroyed_MC2();

	// Blueprint: override to play explosion + swap mesh
	UFUNCTION(BlueprintImplementableEvent, Category = "State")
	void BP_OnDestroyed();

	// --- Destruction assets (set in Blueprint CDO) ---

	// Mesh to swap to when destroyed (the "downed mech" static mesh)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "State|Destruction")
	TObjectPtr<UStaticMesh> DestroyedMesh;

	// Niagara explosion system to spawn at death location
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "State|Destruction")
	TObjectPtr<class UNiagaraSystem> ExplosionEffect;

	// Explosion scale: small=0.5, medium=1.0, large=2.0 (matches MC2 carnage sizes)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "State|Destruction")
	float ExplosionScale = 1.0f;

	// --- Delegates ---

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoverDestroyed, AMC2Mover*, Mover);

	UPROPERTY(BlueprintAssignable, Category = "State")
	FOnMoverDestroyed OnMoverDestroyed;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	FMC2MoverOrder CurrentOrder;

private:
	void UpdateHeat(float DeltaSeconds);
	void UpdateShutdown();
};
