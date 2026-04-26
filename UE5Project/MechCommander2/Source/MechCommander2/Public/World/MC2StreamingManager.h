#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MC2StreamingManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelStreamingComplete, FName, LevelName);

/**
 * UMC2StreamingManager
 * World subsystem that drives async level streaming for MC2's sector-based maps.
 *
 * MC2 maps are split into 9-16 streaming sub-levels (terrain sectors) loaded/unloaded
 * as the camera moves. This class wraps ULevelStreamingDynamic to keep the loading
 * budget under 16 ms / frame.
 *
 * Usage:
 *   Subsystem->RequestLoadLevel("L_Sector_03", PriorityHigh);
 *   Subsystem->RequestUnloadLevel("L_Sector_01");
 *
 * Set up streaming level names in BP_MC2GameMode and call RequestLoadLevel from the
 * mission start sequence.
 */
UCLASS()
class MECHCOMMANDER2_API UMC2StreamingManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Fired when a level finishes loading and is visible
	UPROPERTY(BlueprintAssignable, Category = "Streaming")
	FOnLevelStreamingComplete OnLevelLoaded;

	// Fired when a level has fully unloaded
	UPROPERTY(BlueprintAssignable, Category = "Streaming")
	FOnLevelStreamingComplete OnLevelUnloaded;

	// Load a streaming sublevel by package name (e.g. "/Game/Maps/Sectors/L_Sector_03")
	UFUNCTION(BlueprintCallable, Category = "Streaming")
	void RequestLoadLevel(FName LevelPackageName);

	// Unload a streaming sublevel
	UFUNCTION(BlueprintCallable, Category = "Streaming")
	void RequestUnloadLevel(FName LevelPackageName);

	// Returns true if the level is currently loaded and visible
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Streaming")
	bool IsLevelLoaded(FName LevelPackageName) const;

	// Preload a set of levels (e.g. all sectors adjacent to camera position)
	UFUNCTION(BlueprintCallable, Category = "Streaming")
	void PreloadLevels(const TArray<FName>& LevelNames);

	// Unload all streaming levels except those in KeepLoaded
	UFUNCTION(BlueprintCallable, Category = "Streaming")
	void UnloadAllExcept(const TArray<FName>& KeepLoaded);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }
	virtual TStatId GetStatId() const override;

private:
	// Levels currently requested to be loaded
	TSet<FName> DesiredLoaded;

	// Pending completion checks (level name → frame count waiting)
	TMap<FName, int32> PendingLoadChecks;
	TMap<FName, int32> PendingUnloadChecks;

	static constexpr int32 MAX_WAIT_FRAMES = 300; // ~5s at 60fps before warning

	void PollLoadStatus(float DeltaTime);
};
