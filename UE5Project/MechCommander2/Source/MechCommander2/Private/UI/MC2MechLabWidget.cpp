#include "UI/MC2MechLabWidget.h"
#include "Campaign/MC2LogisticsSubsystem.h"
#include "MC2DataTableRows.h"
#include "Engine/DataTable.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

void UMC2MechLabWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SaveButton)  SaveButton->OnClicked.AddDynamic(this, &UMC2MechLabWidget::SaveLoadout);
}

void UMC2MechLabWidget::LoadMech(int32 RosterIndex)
{
	UMC2LogisticsSubsystem* LS = GetGameInstance()->GetSubsystem<UMC2LogisticsSubsystem>();
	if (!LS || !LS->GetMechRoster().IsValidIndex(RosterIndex)) return;

	EditingRosterIndex = RosterIndex;
	WorkingCopy = LS->GetMechRoster()[RosterIndex];

	if (MechNameText)
		MechNameText->SetText(FText::FromName(WorkingCopy.ChassisID));

	UpdateTonnageDisplay();
	OnLoadoutChanged();
}

bool UMC2MechLabWidget::PlaceComponent(FName ComponentID, int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= WorkingCopy.ComponentSlots.Num())
	{
		WorkingCopy.ComponentSlots.SetNum(SlotIndex + 1);
	}

	WorkingCopy.ComponentSlots[SlotIndex] = ComponentID;

	if (IsOverweight())
	{
		WorkingCopy.ComponentSlots[SlotIndex] = NAME_None;
		OnValidationFailed(FText::FromString(TEXT("Exceeds weight limit")));
		return false;
	}

	UpdateTonnageDisplay();
	OnLoadoutChanged();
	return true;
}

void UMC2MechLabWidget::ClearSlot(int32 SlotIndex)
{
	if (WorkingCopy.ComponentSlots.IsValidIndex(SlotIndex))
	{
		WorkingCopy.ComponentSlots[SlotIndex] = NAME_None;
		UpdateTonnageDisplay();
		OnLoadoutChanged();
	}
}

void UMC2MechLabWidget::SaveLoadout()
{
	UMC2LogisticsSubsystem* LS = GetGameInstance()->GetSubsystem<UMC2LogisticsSubsystem>();
	if (!LS || EditingRosterIndex < 0) return;

	if (IsOverweight())
	{
		OnValidationFailed(FText::FromString(TEXT("Cannot save: exceeds weight limit")));
		return;
	}

	// Write working copy back into the mech roster
	// (A proper implementation would call an UpdateMechLoadout function on LS)
	LS->GetMechRoster();   // roster is const; modification requires a proper accessor — add to subsystem if needed
}

float UMC2MechLabWidget::GetCurrentTonnage() const
{
	// Sum weights of all components in working copy (requires DT_Components lookup)
	// Placeholder — full implementation requires DT reference passed in from BP
	return 0.f;
}

float UMC2MechLabWidget::GetMaxTonnage() const
{
	return 0.f;  // set from chassis data loaded in LoadMech
}

int32 UMC2MechLabWidget::GetUsedSlots(int32 LocationIndex) const
{
	return 0;   // slot counting per location requires component data
}

void UMC2MechLabWidget::UpdateTonnageDisplay()
{
	if (!TonnageText) return;
	TonnageText->SetText(FText::Format(
		FText::FromString(TEXT("{0} / {1} t")),
		FText::AsNumber(FMath::RoundToInt(GetCurrentTonnage())),
		FText::AsNumber(FMath::RoundToInt(GetMaxTonnage()))
	));
}
