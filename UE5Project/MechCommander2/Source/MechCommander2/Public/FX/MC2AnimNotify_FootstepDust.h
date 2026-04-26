#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "FX/MC2FXLibrary.h"
#include "MC2AnimNotify_FootstepDust.generated.h"

/**
 * UMC2AnimNotify_FootstepDust
 * Placed on mech walk/run animation sequences at the foot-plant frame.
 * Spawns NS_MC2_Dust_Cloud at the foot socket location.
 *
 * Add to both left and right foot plant frames in the animation sequence editor.
 * Socket names: "foot_l", "foot_r" (must exist on the mech skeleton).
 */
UCLASS()
class MECHCOMMANDER2_API UMC2AnimNotify_FootstepDust : public UAnimNotify
{
	GENERATED_BODY()

public:
	// Which foot socket to spawn dust at
	UPROPERTY(EditAnywhere, Category = "Footstep")
	FName FootSocket = TEXT("foot_l");

	// Scale relative to mech tonnage — set higher for heavier mechs
	UPROPERTY(EditAnywhere, Category = "Footstep")
	float DustScale = 1.f;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	                    const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override
	{
		return FString::Printf(TEXT("FootstepDust_%s"), *FootSocket.ToString());
	}
};
