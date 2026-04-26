#include "FX/MC2FXLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"

TMap<EMC2FXType, TWeakObjectPtr<UNiagaraSystem>> UMC2FXLibrary::FXCache;

static const TCHAR* FXTypeNames[] = {
	TEXT("Explosion_Small"),
	TEXT("Explosion_Medium"),
	TEXT("Explosion_Large"),
	TEXT("Muzzle_Energy"),
	TEXT("Muzzle_Ballistic"),
	TEXT("Muzzle_Missile"),
	TEXT("Smoke_Trail"),
	TEXT("Fire"),
	TEXT("Dust_Cloud"),
	TEXT("Hit_Spark"),
	TEXT("Water_Splash"),
	TEXT("JumpJet"),
};

FString UMC2FXLibrary::GetAssetPathString(EMC2FXType FXType)
{
	const int32 Idx = (int32)FXType;
	if (Idx < 0 || Idx >= UE_ARRAY_COUNT(FXTypeNames))
		return {};
	const FString Name = FString::Printf(TEXT("NS_MC2_%s"), FXTypeNames[Idx]);
	return FString::Printf(TEXT("/Game/FX/%s.%s"), *Name, *Name);
}

UNiagaraSystem* UMC2FXLibrary::GetOrLoadFX(EMC2FXType FXType)
{
	if (TWeakObjectPtr<UNiagaraSystem>* Cached = FXCache.Find(FXType))
	{
		if (Cached->IsValid())
			return Cached->Get();
	}

	const FString Path = GetAssetPathString(FXType);
	UNiagaraSystem* NS = LoadObject<UNiagaraSystem>(nullptr, *Path);
	FXCache.Add(FXType, NS);
	return NS;
}

UNiagaraComponent* UMC2FXLibrary::SpawnFX(
	const UObject* WorldContext,
	EMC2FXType FXType,
	FVector Location,
	FRotator Rotation,
	float Scale)
{
	UNiagaraSystem* NS = GetOrLoadFX(FXType);
	if (!NS || !WorldContext) return nullptr;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return nullptr;

	return UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World, NS, Location, Rotation, FVector(Scale), true, true
	);
}

UNiagaraComponent* UMC2FXLibrary::SpawnFXAttached(
	EMC2FXType FXType,
	USceneComponent* AttachTo,
	FName SocketName,
	FVector LocationOffset,
	float Scale)
{
	UNiagaraSystem* NS = GetOrLoadFX(FXType);
	if (!NS || !AttachTo) return nullptr;

	return UNiagaraFunctionLibrary::SpawnSystemAttached(
		NS, AttachTo, SocketName,
		LocationOffset, FRotator::ZeroRotator,
		FVector(Scale),
		EAttachLocation::KeepRelativeOffset,
		true
	);
}

FSoftObjectPath UMC2FXLibrary::GetFXAssetPath(EMC2FXType FXType)
{
	return FSoftObjectPath(GetAssetPathString(FXType));
}

float UMC2FXLibrary::GetExplosionScaleForBR(int32 BattleRating)
{
	// GosFX explosion magnitude scales linearly with BattleRating.
	// BR 1 (infantry) = 0.5x, BR 10 (medium mech) = 1.0x, BR 20 (assault) = 2.0x
	return FMath::GetMappedRangeValueClamped(
		FVector2D(1.f, 20.f),
		FVector2D(0.5f, 2.f),
		(float)BattleRating
	);
}
