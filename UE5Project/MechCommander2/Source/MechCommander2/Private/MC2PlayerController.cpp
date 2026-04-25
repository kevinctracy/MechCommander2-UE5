#include "MC2PlayerController.h"
#include "MC2GameState.h"
#include "Units/MC2Mover.h"
#include "Mission/MC2GameMode.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "AIController.h"

AMC2PlayerController::AMC2PlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AMC2PlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	// Late-join: if the mission already started before this controller fully connected,
	// request the ready signal so the loading screen is dismissed correctly.
	if (IsLocalController())
	{
		if (const AMC2GameState* GS = GetWorld() ? GetWorld()->GetGameState<AMC2GameState>() : nullptr)
		{
			if (GS->bMissionReady)
				RequestMissionStateSync();
			else
				Server_RequestMissionSync();
		}
	}
}

void AMC2PlayerController::RequestMissionStateSync()
{
	// GameState already has bMissionReady=true (replicated or set locally).
	// Fire the Blueprint event so HUD can initialize.
	if (AMC2GameState* GS = GetWorld() ? GetWorld()->GetGameState<AMC2GameState>() : nullptr)
		GS->OnMissionReady(GS->bMissionReady ? 1 : 0);
}

void AMC2PlayerController::Server_RequestMissionSync_Implementation()
{
	// Server re-fires the multicast for this specific late-joining client.
	// In practice the multicast is reliable so this handles race conditions
	// where BeginPlay fires before the initial multicast arrives.
	if (AMC2GameState* GS = GetWorld() ? GetWorld()->GetGameState<AMC2GameState>() : nullptr)
	{
		if (GS->bMissionReady)
		{
			AMC2GameMode* GM = GetWorld()->GetAuthGameMode<AMC2GameMode>();
			GS->Multicast_OnMissionReady(GM ? GM->GetAllUnits().Num() : 0);
		}
	}
}

void AMC2PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (IA_SelectPress)   EIC->BindAction(IA_SelectPress,   ETriggerEvent::Triggered, this, &AMC2PlayerController::OnSelectPressed);
		if (IA_SelectRelease) EIC->BindAction(IA_SelectRelease, ETriggerEvent::Triggered, this, &AMC2PlayerController::OnSelectReleased);
		if (IA_OrderRight)    EIC->BindAction(IA_OrderRight,    ETriggerEvent::Triggered, this, &AMC2PlayerController::OnOrderRight);
	}
}

void AMC2PlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (bBoxSelecting)
		UpdateBoxSelect();
}

void AMC2PlayerController::OnSelectPressed()
{
	float MouseX, MouseY;
	GetMousePosition(MouseX, MouseY);
	BoxSelectStart = FVector2D(MouseX, MouseY);
	bBoxSelecting = true;
}

void AMC2PlayerController::OnSelectReleased()
{
	if (!bBoxSelecting)
		return;
	bBoxSelecting = false;

	float MouseX, MouseY;
	GetMousePosition(MouseX, MouseY);
	BoxSelectCurrent = FVector2D(MouseX, MouseY);

	FinishBoxSelect();
}

void AMC2PlayerController::UpdateBoxSelect()
{
	float MouseX, MouseY;
	GetMousePosition(MouseX, MouseY);
	BoxSelectCurrent = FVector2D(MouseX, MouseY);
}

void AMC2PlayerController::FinishBoxSelect()
{
	// Deproject screen corners to world frustum and collect actors
	FVector2D MinPt = FVector2D(FMath::Min(BoxSelectStart.X, BoxSelectCurrent.X),
	                            FMath::Min(BoxSelectStart.Y, BoxSelectCurrent.Y));
	FVector2D MaxPt = FVector2D(FMath::Max(BoxSelectStart.X, BoxSelectCurrent.X),
	                            FMath::Max(BoxSelectStart.Y, BoxSelectCurrent.Y));

	bool bSingleClick = (MaxPt - MinPt).SizeSquared() < 25.f;  // < 5px drag = single click

	TArray<AMC2Mover*> NewSelection;

	if (bSingleClick)
	{
		FVector WorldLoc;
		AActor* HitActor;
		if (GetCursorWorldHit(WorldLoc, HitActor))
		{
			if (AMC2Mover* Mover = Cast<AMC2Mover>(HitActor))
			{
				if (Mover->TeamIndex == PlayerTeamIndex)
					NewSelection.Add(Mover);
			}
		}
	}
	else
	{
		// Box select: iterate all friendly movers, check if their screen position is inside box
		TArray<AActor*> AllMovers;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMC2Mover::StaticClass(), AllMovers);
		for (AActor* Actor : AllMovers)
		{
			AMC2Mover* Mover = Cast<AMC2Mover>(Actor);
			if (!Mover || Mover->TeamIndex != PlayerTeamIndex)
				continue;

			FVector2D ScreenPos;
			if (ProjectWorldLocationToScreen(Mover->GetActorLocation(), ScreenPos, true))
			{
				if (ScreenPos.X >= MinPt.X && ScreenPos.X <= MaxPt.X &&
				    ScreenPos.Y >= MinPt.Y && ScreenPos.Y <= MaxPt.Y)
				{
					NewSelection.Add(Mover);
				}
			}
		}
	}

	SelectUnits(NewSelection);
}

void AMC2PlayerController::OnOrderRight()
{
	if (SelectedUnits.IsEmpty())
		return;

	FVector WorldLoc;
	AActor* HitActor;
	if (!GetCursorWorldHit(WorldLoc, HitActor))
		return;

	// Attack order if cursor is on an enemy unit
	if (AMC2Mover* TargetMover = Cast<AMC2Mover>(HitActor))
	{
		if (TargetMover->TeamIndex != PlayerTeamIndex)
		{
			IssueAttackOrder(HitActor);
			return;
		}
	}

	IssueMoveOrder(WorldLoc);
}

void AMC2PlayerController::IssueMoveOrder(const FVector& WorldTarget)
{
	// Compute move direction from first unit to target for formation
	FVector LeadUnit = SelectedUnits.IsEmpty() ? WorldTarget : SelectedUnits[0]->GetActorLocation();
	FVector MoveDir  = (WorldTarget - LeadUnit).GetSafeNormal2D();

	for (int32 i = 0; i < SelectedUnits.Num(); ++i)
	{
		AMC2Mover* Mover = SelectedUnits[i];
		if (!IsValid(Mover))
			continue;

		FVector Destination = WorldTarget + FormationOffset(i, SelectedUnits.Num(), MoveDir);

		// Route through MC2 mover's order system (calls its AI controller)
		Mover->ReceiveMoveOrder(Destination);
	}
}

void AMC2PlayerController::IssueAttackOrder(AActor* Target)
{
	for (AMC2Mover* Mover : SelectedUnits)
	{
		if (IsValid(Mover))
			Mover->ReceiveAttackOrder(Target);
	}
}

void AMC2PlayerController::IssueGuardOrder(const FVector& WorldTarget)
{
	for (AMC2Mover* Mover : SelectedUnits)
	{
		if (IsValid(Mover))
			Mover->ReceiveGuardOrder(WorldTarget);
	}
}

void AMC2PlayerController::SelectUnits(const TArray<AMC2Mover*>& Units)
{
	// Deselect old units
	for (AMC2Mover* Mover : SelectedUnits)
	{
		if (IsValid(Mover))
			Mover->SetSelected(false);
	}

	SelectedUnits = Units;

	for (AMC2Mover* Mover : SelectedUnits)
	{
		if (IsValid(Mover))
			Mover->SetSelected(true);
	}

	OnUnitsSelected.Broadcast(SelectedUnits);
}

void AMC2PlayerController::ClearSelection()
{
	SelectUnits({});
}

bool AMC2PlayerController::GetCursorWorldHit(FVector& OutLocation, AActor*& OutActor) const
{
	FHitResult Hit;
	GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), true, Hit);
	if (Hit.bBlockingHit)
	{
		OutLocation = Hit.Location;
		OutActor    = Hit.GetActor();
		return true;
	}
	return false;
}

FVector AMC2PlayerController::FormationOffset(int32 UnitIndex, int32 TotalUnits, const FVector& MoveDir) const
{
	if (TotalUnits <= 1)
		return FVector::ZeroVector;

	// Simple grid formation: rows of 4, units spaced 350 UU apart
	const float Spacing = 350.f;
	int32 Col = UnitIndex % 4;
	int32 Row = UnitIndex / 4;

	// Right vector perpendicular to move direction
	FVector Right = FVector::CrossProduct(MoveDir, FVector::UpVector).GetSafeNormal();

	float OffsetRight = (Col - 1.5f) * Spacing;
	float OffsetBack  = Row * -Spacing;

	return Right * OffsetRight + MoveDir * OffsetBack;
}
