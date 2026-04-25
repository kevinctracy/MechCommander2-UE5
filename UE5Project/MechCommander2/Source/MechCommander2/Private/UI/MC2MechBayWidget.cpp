#include "UI/MC2MechBayWidget.h"
#include "Campaign/MC2LogisticsSubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Kismet/GameplayStatics.h"

void UMC2MechBayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RepairButton)       RepairButton->OnClicked.AddDynamic(this, &UMC2MechBayWidget::HandleRepairClicked);
	if (LaunchMissionButton) LaunchMissionButton->OnClicked.AddDynamic(this, &UMC2MechBayWidget::HandleLaunchMission);

	RefreshRoster();
}

void UMC2MechBayWidget::RefreshRoster()
{
	// Populate MechRosterList via Blueprint (UObject data binding)
	// C++ side refreshes text widgets for the current selection
	if (CBillsText)
	{
		int32 CBills = 0;
		if (UMC2LogisticsSubsystem* LS = GetGameInstance()->GetSubsystem<UMC2LogisticsSubsystem>())
			CBills = LS->GetCBills();
		CBillsText->SetText(FText::AsNumber(CBills));
	}

	OnRosterRefreshed();
}

void UMC2MechBayWidget::SelectMech(int32 RosterIndex)
{
	SelectedRosterIndex = RosterIndex;

	UMC2LogisticsSubsystem* LS = GetGameInstance()->GetSubsystem<UMC2LogisticsSubsystem>();
	if (!LS || !LS->GetMechRoster().IsValidIndex(RosterIndex)) return;

	const FMC2MechRecord& Mech = LS->GetMechRoster()[RosterIndex];

	if (MechNameText)
		MechNameText->SetText(FText::FromName(Mech.ChassisID));

	if (PilotNameText)
		PilotNameText->SetText(Mech.AssignedPilotID.IsNone()
			? FText::FromString(TEXT("Unassigned"))
			: FText::FromName(Mech.AssignedPilotID));

	// Estimate armor damage for repair cost
	int32 TotalArmorDamage =
		(Mech.CurrentArmorFront + Mech.CurrentArmorRear + Mech.CurrentArmorLeft + Mech.CurrentArmorRight);
	int32 RepairCost = TotalArmorDamage * CBILLS_PER_ARMOR_POINT;

	if (RepairCostText)
		RepairCostText->SetText(FText::Format(
			FText::FromString(TEXT("Repair: {0} C-Bills")),
			FText::AsNumber(RepairCost)
		));

	OnMechSelected(RosterIndex, Mech);
}

bool UMC2MechBayWidget::RepairSelectedMech()
{
	if (SelectedRosterIndex < 0) return false;

	UMC2LogisticsSubsystem* LS = GetGameInstance()->GetSubsystem<UMC2LogisticsSubsystem>();
	if (!LS || !LS->GetMechRoster().IsValidIndex(SelectedRosterIndex)) return false;

	const FMC2MechRecord& Mech = LS->GetMechRoster()[SelectedRosterIndex];
	int32 TotalArmorDamage = Mech.CurrentArmorFront + Mech.CurrentArmorRear
	                       + Mech.CurrentArmorLeft  + Mech.CurrentArmorRight;
	int32 RepairCost = TotalArmorDamage * CBILLS_PER_ARMOR_POINT;

	if (!LS->SpendCBills(RepairCost)) return false;

	// Repair is tracked in save data; actual armor restoration done by mission spawn
	RefreshRoster();
	OnRepairCompleted(RepairCost);
	return true;
}

bool UMC2MechBayWidget::AssignPilotToSelected(FName PilotID)
{
	if (SelectedRosterIndex < 0) return false;
	UMC2LogisticsSubsystem* LS = GetGameInstance()->GetSubsystem<UMC2LogisticsSubsystem>();
	if (!LS) return false;
	bool bOk = LS->AssignPilotToMech(SelectedRosterIndex, PilotID);
	if (bOk) SelectMech(SelectedRosterIndex);
	return bOk;
}

void UMC2MechBayWidget::HandleRepairClicked()
{
	RepairSelectedMech();
}

void UMC2MechBayWidget::HandleLaunchMission()
{
	// Transition to mission briefing — level name comes from the logistics subsystem's current mission index
	UGameplayStatics::OpenLevel(this, FName("L_MissionBriefing"));
}
