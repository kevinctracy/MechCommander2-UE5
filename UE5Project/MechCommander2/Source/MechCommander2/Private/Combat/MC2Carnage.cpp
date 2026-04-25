#include "Combat/MC2Carnage.h"
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

AMC2Carnage::AMC2Carnage()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	NiagaraComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Niagara"));
	SetRootComponent(NiagaraComp);
	NiagaraComp->bAutoActivate = false;

	AudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	AudioComp->SetupAttachment(NiagaraComp);
	AudioComp->bAutoActivate = false;
}

void AMC2Carnage::BeginPlay()
{
	Super::BeginPlay();

	NiagaraComp->OnSystemFinished.AddDynamic(this, &AMC2Carnage::OnNiagaraFinished);
	NiagaraComp->ActivateSystem();

	if (AudioComp->Sound)
		AudioComp->Play();

	if (Duration > 0.f)
		SetLifeSpan(Duration);
}

void AMC2Carnage::OnNiagaraFinished(UNiagaraComponent* PSystem)
{
	// Let the actor live its natural duration; destroy only if no explicit duration
	if (Duration <= 0.f)
		Destroy();
}

AMC2Carnage* AMC2Carnage::SpawnCarnage(
	UObject* WorldContext,
	TSubclassOf<AMC2Carnage> CarnageClass,
	const FVector& Location,
	EMC2CarnageType Type)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
	if (!World || !CarnageClass)
		return nullptr;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMC2Carnage* Carnage = World->SpawnActor<AMC2Carnage>(CarnageClass, Location, FRotator::ZeroRotator, Params);
	if (Carnage)
		Carnage->CarnageType = Type;
	return Carnage;
}
