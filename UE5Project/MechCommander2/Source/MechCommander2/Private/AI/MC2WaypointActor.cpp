#include "AI/MC2WaypointActor.h"
#include "Components/BillboardComponent.h"

AMC2WaypointActor::AMC2WaypointActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bIsEditorOnlyActor = false;

#if WITH_EDITORONLY_DATA
	// Show an editor billboard so designers can see the waypoints
	UBillboardComponent* Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	SetRootComponent(Billboard);
#endif
}

void AMC2WaypointActor::BeginPlay()
{
	Super::BeginPlay();
}

AMC2WaypointActor* AMC2WaypointActor::GetChainStart()
{
	// Walk backwards — if no reverse links, just return self
	return this;
}

#if WITH_EDITORONLY_DATA
void AMC2WaypointActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
