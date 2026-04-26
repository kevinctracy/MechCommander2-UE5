#include "World/MC2StreamingManager.h"
#include "Engine/World.h"
#include "Engine/LevelStreaming.h"
#include "Engine/LevelStreamingDynamic.h"

DEFINE_STAT(STAT_MC2StreamingManager);

void UMC2StreamingManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

TStatId UMC2StreamingManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMC2StreamingManager, STATGROUP_Tickables);
}

void UMC2StreamingManager::RequestLoadLevel(FName LevelPackageName)
{
	if (DesiredLoaded.Contains(LevelPackageName)) { return; }
	DesiredLoaded.Add(LevelPackageName);

	UWorld* World = GetWorld();
	if (!World) { return; }

	// Check if already a streaming level instance
	for (ULevelStreaming* LS : World->GetStreamingLevels())
	{
		if (LS && LS->GetWorldAssetPackageFName() == LevelPackageName)
		{
			LS->SetShouldBeLoaded(true);
			LS->SetShouldBeVisible(true);
			PendingLoadChecks.Add(LevelPackageName, 0);
			return;
		}
	}

	// Dynamically add it
	bool bSuccess = false;
	ULevelStreamingDynamic* NewStreaming = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
		World, TSoftObjectPtr<UWorld>(FSoftObjectPath(LevelPackageName.ToString())),
		FVector::ZeroVector, FRotator::ZeroRotator, bSuccess);

	if (bSuccess && NewStreaming)
	{
		NewStreaming->SetShouldBeLoaded(true);
		NewStreaming->SetShouldBeVisible(true);
		PendingLoadChecks.Add(LevelPackageName, 0);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[MC2Stream] Failed to begin loading level: %s"), *LevelPackageName.ToString());
	}
}

void UMC2StreamingManager::RequestUnloadLevel(FName LevelPackageName)
{
	DesiredLoaded.Remove(LevelPackageName);

	UWorld* World = GetWorld();
	if (!World) { return; }

	for (ULevelStreaming* LS : World->GetStreamingLevels())
	{
		if (LS && LS->GetWorldAssetPackageFName() == LevelPackageName)
		{
			LS->SetShouldBeLoaded(false);
			LS->SetShouldBeVisible(false);
			PendingUnloadChecks.Add(LevelPackageName, 0);
			return;
		}
	}
}

bool UMC2StreamingManager::IsLevelLoaded(FName LevelPackageName) const
{
	UWorld* World = GetWorld();
	if (!World) { return false; }

	for (const ULevelStreaming* LS : World->GetStreamingLevels())
	{
		if (LS && LS->GetWorldAssetPackageFName() == LevelPackageName)
		{
			return LS->IsLevelLoaded() && LS->IsLevelVisible();
		}
	}
	return false;
}

void UMC2StreamingManager::PreloadLevels(const TArray<FName>& LevelNames)
{
	for (const FName& Name : LevelNames)
	{
		RequestLoadLevel(Name);
	}
}

void UMC2StreamingManager::UnloadAllExcept(const TArray<FName>& KeepLoaded)
{
	TSet<FName> Keep(KeepLoaded);
	TArray<FName> ToUnload;
	for (const FName& Loaded : DesiredLoaded)
	{
		if (!Keep.Contains(Loaded))
		{
			ToUnload.Add(Loaded);
		}
	}
	for (const FName& Name : ToUnload)
	{
		RequestUnloadLevel(Name);
	}
}

void UMC2StreamingManager::Tick(float DeltaTime)
{
	PollLoadStatus(DeltaTime);
}

void UMC2StreamingManager::PollLoadStatus(float DeltaTime)
{
	// Check pending loads
	TArray<FName> LoadCompleted;
	for (auto& KV : PendingLoadChecks)
	{
		if (IsLevelLoaded(KV.Key))
		{
			LoadCompleted.Add(KV.Key);
			OnLevelLoaded.Broadcast(KV.Key);
		}
		else
		{
			KV.Value++;
			if (KV.Value > MAX_WAIT_FRAMES)
			{
				UE_LOG(LogTemp, Warning, TEXT("[MC2Stream] Level %s taking long to load (%d frames)"),
					*KV.Key.ToString(), KV.Value);
			}
		}
	}
	for (const FName& Name : LoadCompleted) { PendingLoadChecks.Remove(Name); }

	// Check pending unloads
	TArray<FName> UnloadCompleted;
	for (auto& KV : PendingUnloadChecks)
	{
		if (!IsLevelLoaded(KV.Key))
		{
			UnloadCompleted.Add(KV.Key);
			OnLevelUnloaded.Broadcast(KV.Key);
		}
		else
		{
			KV.Value++;
		}
	}
	for (const FName& Name : UnloadCompleted) { PendingUnloadChecks.Remove(Name); }
}
