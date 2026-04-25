#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MC2PlayerController.generated.h"

class AMC2Mover;
class UInputMappingContext;
class UInputAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitsSelected, const TArray<AMC2Mover*>&, SelectedUnits);

/**
 * AMC2PlayerController
 * Handles top-down RTS input: box select, right-click move orders, camera control.
 * Mirrors the player command interface from MC2's warrior.h / mission GUI.
 */
UCLASS()
class MECHCOMMANDER2_API AMC2PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMC2PlayerController();

	// --- Selection ---

	UPROPERTY(BlueprintAssignable, Category = "Selection")
	FOnUnitsSelected OnUnitsSelected;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Selection")
	const TArray<AMC2Mover*>& GetSelectedUnits() const { return SelectedUnits; }

	UFUNCTION(BlueprintCallable, Category = "Selection")
	void SelectUnits(const TArray<AMC2Mover*>& Units);

	UFUNCTION(BlueprintCallable, Category = "Selection")
	void ClearSelection();

	// --- Orders ---

	// Issue move order to all selected units (right-click ground).
	UFUNCTION(BlueprintCallable, Category = "Orders")
	void IssueMoveOrder(const FVector& WorldTarget);

	// Issue attack order on a specific unit.
	UFUNCTION(BlueprintCallable, Category = "Orders")
	void IssueAttackOrder(AActor* Target);

	// Issue guard order at a position.
	UFUNCTION(BlueprintCallable, Category = "Orders")
	void IssueGuardOrder(const FVector& WorldTarget);

	// Player team index (0 = player, 1+ = enemies/allies).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team")
	int32 PlayerTeamIndex = 0;

	// --- Late-join sync ---
	// Called automatically on BeginPlay if the mission is already in progress
	// (bMissionReady is already true on GameState). Allows late joiners to
	// skip the loading screen and jump straight into the mission.
	UFUNCTION(BlueprintCallable, Category = "Mission")
	void RequestMissionStateSync();

	// Server RPC: ask the server to re-fire Multicast_OnMissionReady for this client.
	UFUNCTION(Server, Reliable, Category = "Mission")
	void Server_RequestMissionSync();

	// --- Input ---

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_SelectPress;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_SelectRelease;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_OrderRight;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_CameraMove;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_CameraZoom;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_CameraRotate;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	// Box selection state
	bool bBoxSelecting = false;
	FVector2D BoxSelectStart;
	FVector2D BoxSelectCurrent;

	TArray<AMC2Mover*> SelectedUnits;

	void OnSelectPressed();
	void OnSelectReleased();
	void OnOrderRight();
	void UpdateBoxSelect();
	void FinishBoxSelect();

	// Get world-space hit under cursor
	bool GetCursorWorldHit(FVector& OutLocation, AActor*& OutActor) const;

	// Formation offset for multi-unit move orders
	FVector FormationOffset(int32 UnitIndex, int32 TotalUnits, const FVector& MoveDir) const;
};
