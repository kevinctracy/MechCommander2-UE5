#include "Camera/MC2CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/LocalPlayer.h"

AMC2CameraActor::AMC2CameraActor()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraRoot"));
	SetRootComponent(CameraRoot);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(CameraRoot);
	SpringArm->TargetArmLength    = 1200.f;
	SpringArm->bDoCollisionTest   = false;
	SpringArm->bEnableCameraLag   = false;
	SpringArm->SetRelativeRotation(FRotator(-55.f, 0.f, 0.f));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	TargetZoom       = 1200.f;
	TargetYaw        = 0.f;
	TargetPitch      = -55.f;
}

void AMC2CameraActor::BeginPlay()
{
	Super::BeginPlay();
	TargetPanLocation = GetActorLocation();
}

void AMC2CameraActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ApplyEdgeScroll(DeltaSeconds);

	// Smooth pan
	FVector NewLoc = FMath::VInterpTo(GetActorLocation(), TargetPanLocation, DeltaSeconds, PanInterpSpeed);
	SetActorLocation(NewLoc);

	// Smooth zoom
	float NewZoom = FMath::FInterpTo(SpringArm->TargetArmLength, TargetZoom, DeltaSeconds, ZoomInterpSpeed);
	SpringArm->TargetArmLength = NewZoom;

	// Smooth rotation
	float NewYaw   = FMath::FInterpTo(GetActorRotation().Yaw,   TargetYaw,   DeltaSeconds, 8.f);
	float NewPitch = FMath::FInterpTo(SpringArm->GetRelativeRotation().Pitch, TargetPitch, DeltaSeconds, 8.f);
	SetActorRotation(FRotator(0.f, NewYaw, 0.f));
	SpringArm->SetRelativeRotation(FRotator(NewPitch, 0.f, 0.f));
}

void AMC2CameraActor::PanInput(FVector2D Delta)
{
	if (Delta.IsNearlyZero())
		return;

	auto [Forward, Right] = GetForwardRight();  // structured binding

	float SpeedScale = SpringArm->TargetArmLength / MaxZoomDistance;  // pan faster when zoomed out
	FVector Move = Right * Delta.X * KeyPanSpeed * SpeedScale
	             + Forward * Delta.Y * KeyPanSpeed * SpeedScale;
	TargetPanLocation += Move;
}

void AMC2CameraActor::ZoomInput(float Delta)
{
	TargetZoom = FMath::Clamp(TargetZoom - Delta * ZoomStep, MinZoomDistance, MaxZoomDistance);
}

void AMC2CameraActor::RotateInput(FVector2D Delta)
{
	TargetYaw   += Delta.X * RotateSpeed;
	TargetPitch  = FMath::Clamp(TargetPitch + Delta.Y * RotateSpeed * 0.5f, MinPitch, MaxPitch);
}

void AMC2CameraActor::FocusOnLocation(const FVector& WorldLocation, bool bInstant)
{
	TargetPanLocation = WorldLocation;
	if (bInstant)
		SetActorLocation(WorldLocation);
}

void AMC2CameraActor::ApplyEdgeScroll(float DeltaSeconds)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
		return;

	int32 ViewX, ViewY;
	PC->GetViewportSize(ViewX, ViewY);

	float MouseX, MouseY;
	PC->GetMousePosition(MouseX, MouseY);

	FVector2D ScrollDir = FVector2D::ZeroVector;
	if (MouseX < EdgeScrollMargin) ScrollDir.X = -1.f;
	else if (MouseX > ViewX - EdgeScrollMargin) ScrollDir.X = 1.f;
	if (MouseY < EdgeScrollMargin) ScrollDir.Y = 1.f;
	else if (MouseY > ViewY - EdgeScrollMargin) ScrollDir.Y = -1.f;

	if (!ScrollDir.IsNearlyZero())
	{
		auto [Forward, Right] = GetForwardRight();
		float SpeedScale = SpringArm->TargetArmLength / MaxZoomDistance;
		TargetPanLocation += Right   * ScrollDir.X * EdgeScrollSpeed * SpeedScale * DeltaSeconds
		                   + Forward * ScrollDir.Y * EdgeScrollSpeed * SpeedScale * DeltaSeconds;
	}
}

std::pair<FVector, FVector> AMC2CameraActor::GetForwardRight() const
{
	float YawRad = FMath::DegreesToRadians(GetActorRotation().Yaw);
	FVector Fwd(FMath::Cos(YawRad), FMath::Sin(YawRad), 0.f);
	FVector Rgt(-FMath::Sin(YawRad), FMath::Cos(YawRad), 0.f);
	return {Fwd, Rgt};
}
