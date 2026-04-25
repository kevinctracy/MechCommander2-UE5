#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MC2CameraActor.generated.h"

class UCameraComponent;
class USpringArmComponent;

/**
 * AMC2CameraActor
 * Top-down RTS camera with:
 *   - WASD + screen-edge scroll pan
 *   - Scroll wheel zoom (arm length 200-2000 UU)
 *   - Middle mouse rotate (yaw + pitch clamped)
 *   - Smooth interpolation on all axes
 *
 * MC2's original camera was a similar top-down perspective with zoom/pan.
 * FOV range from mech3d.h: startFOV / zoomFOV variables.
 */
UCLASS()
class MECHCOMMANDER2_API AMC2CameraActor : public AActor
{
	GENERATED_BODY()

public:
	AMC2CameraActor();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USceneComponent> CameraRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	// --- Zoom ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Zoom")
	float MinZoomDistance = 400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Zoom")
	float MaxZoomDistance = 2500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Zoom")
	float ZoomStep = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Zoom")
	float ZoomInterpSpeed = 8.f;

	// --- Pan ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Pan")
	float KeyPanSpeed = 2000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Pan")
	float EdgeScrollMargin = 20.f;     // pixels from screen edge

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Pan")
	float EdgeScrollSpeed = 2000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Pan")
	float PanInterpSpeed = 10.f;

	// --- Rotation ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Rotation")
	float RotateSpeed = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Rotation")
	float MinPitch = -75.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Rotation")
	float MaxPitch = -20.f;

	// --- API ---

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void PanInput(FVector2D Delta);    // called by PlayerController each frame

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ZoomInput(float Delta);

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void RotateInput(FVector2D Delta);

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void FocusOnLocation(const FVector& WorldLocation, bool bInstant = false);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	float TargetZoom;
	FVector TargetPanLocation;
	float TargetYaw;
	float TargetPitch;

	void ApplyEdgeScroll(float DeltaSeconds);
	FVector2D GetForwardRight() const;  // camera-relative forward/right in world XY
};
