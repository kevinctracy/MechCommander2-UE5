#include "FX/MC2AnimNotify_FootstepDust.h"
#include "FX/MC2FXLibrary.h"

void UMC2AnimNotify_FootstepDust::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;

	// Get foot socket world location; fall back to actor root if socket missing
	FVector SpawnLoc = MeshComp->DoesSocketExist(FootSocket)
		? MeshComp->GetSocketLocation(FootSocket)
		: MeshComp->GetComponentLocation();

	// Spawn at ground level — offset down slightly so dust rises from terrain
	SpawnLoc.Z -= 20.f;

	UMC2FXLibrary::SpawnFX(MeshComp, EMC2FXType::DustCloud, SpawnLoc,
	                       FRotator::ZeroRotator, DustScale);
}
