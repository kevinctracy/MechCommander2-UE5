#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "Engine/DataAsset.h"
#include "MC2InputConfig.generated.h"

/**
 * UMC2InputConfig
 * Data asset that lists all Input Actions used by MC2.
 * Set as the project's input config asset in DefaultEngine.ini.
 * WBP_Options reads this to build the keybinding UI rows.
 *
 * Each InputAction asset lives at /Game/Input/Actions/IA_*.
 * Axis actions (camera scroll, zoom) use 1D/2D value types.
 * Button actions (select, attack, pause) use Digital (bool) type.
 */
UCLASS(BlueprintType)
class MECHCOMMANDER2_API UMC2InputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// ---- Camera ----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UInputAction> IA_CameraPan;      // WASD / middle-mouse drag

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UInputAction> IA_CameraZoom;     // Mouse wheel

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UInputAction> IA_CameraRotate;   // Q/E or right-mouse drag

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UInputAction> IA_CameraEdgeScroll; // Cursor near screen edge

	// ---- Selection ----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection")
	TObjectPtr<UInputAction> IA_SelectPrimary;  // Left click (select unit)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection")
	TObjectPtr<UInputAction> IA_SelectAdditive; // Shift + left click

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection")
	TObjectPtr<UInputAction> IA_SelectBox;      // Left drag (rubber band)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection")
	TObjectPtr<UInputAction> IA_SelectAll;      // Ctrl+A

	// ---- Orders ----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orders")
	TObjectPtr<UInputAction> IA_OrderMove;      // Right click on ground

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orders")
	TObjectPtr<UInputAction> IA_OrderAttack;    // Right click on enemy

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orders")
	TObjectPtr<UInputAction> IA_OrderAttackMove;// A key + left click

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orders")
	TObjectPtr<UInputAction> IA_OrderGuard;     // G key

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orders")
	TObjectPtr<UInputAction> IA_OrderStop;      // S key

	// ---- Control groups ----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ControlGroups")
	TObjectPtr<UInputAction> IA_AssignGroup;    // Ctrl+1-9

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ControlGroups")
	TObjectPtr<UInputAction> IA_SelectGroup;    // 1-9

	// ---- UI ----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UInputAction> IA_Pause;          // P or Escape

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UInputAction> IA_TacMap;         // Tab

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UInputAction> IA_MechBay;        // M (during debrief only)

	// ---- Misc ----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Misc")
	TObjectPtr<UInputAction> IA_Jump;           // Spacebar (jump jet mechs)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Misc")
	TObjectPtr<UInputAction> IA_Screenshot;     // F12

	// Returns the display name for an action (used by WBP_Options to label rows)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Input")
	static FText GetActionDisplayName(const UInputAction* Action);
};
