#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerVolume.h"
#include "MC2MissionVolume.generated.h"

class AMC2Mover;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMoverEnteredVolume,  AMC2MissionVolume*, Volume, AMC2Mover*, Mover);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMoverExitedVolume,   AMC2MissionVolume*, Volume, AMC2Mover*, Mover);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (FOnTeamFullyInsideVolume, int32, TeamIndex);

/**
 * AMC2MissionVolume
 * ATriggerVolume subclass used by MoveUnitToArea and other area-based objective types.
 * Fires Blueprint delegates when mechs/vehicles enter or exit.
 * Named in the editor and referenced by mission script Blueprints by name.
 *
 * Maps to MC2's ABL `isTriggerAreaHit(N)` function — each volume is a named area.
 * Editor placement: place in level, set AreaID, reference from BP_MC2Mission_XX.
 */
UCLASS(BlueprintType, Blueprintable)
class MECHCOMMANDER2_API AMC2MissionVolume : public ATriggerVolume
{
	GENERATED_BODY()

public:
	AMC2MissionVolume();

	// Unique name for this area — matches the ABL trigger area index.
	// Set per-volume in editor. BP_MC2Mission uses this to find the volume.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FName AreaID;

	// If set, only movers on this team trigger the volume (TeamIndex -1 = any team).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	int32 FilterTeamIndex = -1;

	// How many friendly units must be inside simultaneously to fire OnTeamFullyInside.
	// Set to 1 to trigger on first entry.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	int32 RequiredUnitCount = 1;

	// --- Events (bind in BP_MC2Mission) ---

	UPROPERTY(BlueprintAssignable, Category = "Mission|Events")
	FOnMoverEnteredVolume OnMoverEntered;

	UPROPERTY(BlueprintAssignable, Category = "Mission|Events")
	FOnMoverExitedVolume OnMoverExited;

	// Fires when RequiredUnitCount friendly units are simultaneously inside.
	UPROPERTY(BlueprintAssignable, Category = "Mission|Events")
	FOnTeamFullyInsideVolume OnTeamFullyInside;

	// --- Queries ---

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission")
	int32 GetUnitCountInside(int32 TeamIndex) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission")
	bool IsUnitInside(AMC2Mover* Mover) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission")
	const TArray<AMC2Mover*>& GetUnitsInside() const { return UnitsInside; }

protected:
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

private:
	UPROPERTY()
	TArray<TObjectPtr<AMC2Mover>> UnitsInside;

	void CheckTeamThreshold(int32 TeamIndex);
};
