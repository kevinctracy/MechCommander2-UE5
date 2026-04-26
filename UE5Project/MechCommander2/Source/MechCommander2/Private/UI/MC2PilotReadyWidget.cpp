#include "UI/MC2PilotReadyWidget.h"
#include "Campaign/MC2LogisticsSubsystem.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

void UMC2PilotReadyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (LaunchButton) LaunchButton->OnClicked.AddDynamic(this, &UMC2PilotReadyWidget::HandleLaunch);
	if (BackButton)   BackButton->OnClicked.AddDynamic(this, &UMC2PilotReadyWidget::HandleBack);

	if (LaunchButton) LaunchButton->SetIsEnabled(false);
}

void UMC2PilotReadyWidget::SetMissionID(int32 MissionIndex)
{
	CurrentMissionIndex = MissionIndex;
	LanceSlotPilots.Init(-1, 4);
	LanceSlotMechs.Init(-1, 4);

	// Mission name set from Data Table in Blueprint via GetMissionDisplayName()
	// PilotList and MechSlotList populated by BP after this call
}

void UMC2PilotReadyWidget::AssignPilotToSlot(int32 SlotIndex, int32 PilotID)
{
	if (!LanceSlotPilots.IsValidIndex(SlotIndex)) { return; }

	UMC2LogisticsSubsystem* LS = GetGameInstance()->GetSubsystem<UMC2LogisticsSubsystem>();
	if (!LS) { return; }

	// Find which mech this pilot is currently in the roster for
	const int32 MechIdx = LS->GetMechIndexForPilot(PilotID);

	// Clear any slot that already has this pilot
	for (int32 i = 0; i < LanceSlotPilots.Num(); ++i)
	{
		if (i != SlotIndex && LanceSlotPilots[i] == PilotID)
		{
			LanceSlotPilots[i] = -1;
			LanceSlotMechs[i]  = -1;
			OnSlotCleared(i);
		}
	}

	LanceSlotPilots[SlotIndex] = PilotID;
	LanceSlotMechs[SlotIndex]  = MechIdx;

	OnSlotAssigned(SlotIndex, PilotID, MechIdx);

	if (IsLanceReady())
	{
		if (LaunchButton) LaunchButton->SetIsEnabled(true);
		OnLanceReady();
	}
}

void UMC2PilotReadyWidget::ClearSlot(int32 SlotIndex)
{
	if (!LanceSlotPilots.IsValidIndex(SlotIndex)) { return; }
	LanceSlotPilots[SlotIndex] = -1;
	LanceSlotMechs[SlotIndex]  = -1;
	if (LaunchButton) LaunchButton->SetIsEnabled(false);
	OnSlotCleared(SlotIndex);
}

bool UMC2PilotReadyWidget::IsLanceReady() const
{
	// At least one filled slot is enough to launch (MC2 allows solo play)
	return LanceSlotPilots.ContainsByPredicate([](int32 ID) { return ID != -1; });
}

void UMC2PilotReadyWidget::LaunchMission()
{
	UMC2LogisticsSubsystem* LS = GetGameInstance()->GetSubsystem<UMC2LogisticsSubsystem>();
	if (LS)
	{
		// Commit pilot→mech assignments to save before travel
		for (int32 i = 0; i < LanceSlotPilots.Num(); ++i)
		{
			if (LanceSlotPilots[i] != -1 && LanceSlotMechs[i] != -1)
			{
				LS->AssignPilotToMech(LanceSlotPilots[i], LanceSlotMechs[i]);
			}
		}
		LS->SaveGame(0);
	}

	const FName LevelName = FName(FString::Printf(TEXT("L_MC2_%02d"), CurrentMissionIndex));
	UGameplayStatics::OpenLevel(this, LevelName);
}

void UMC2PilotReadyWidget::HandleLaunch() { LaunchMission(); }
void UMC2PilotReadyWidget::HandleBack()   { RemoveFromParent(); }
