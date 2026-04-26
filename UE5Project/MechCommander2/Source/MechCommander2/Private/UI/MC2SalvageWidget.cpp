#include "UI/MC2SalvageWidget.h"
#include "Campaign/MC2LogisticsSubsystem.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Button.h"

void UMC2SalvageWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ConfirmButton) ConfirmButton->OnClicked.AddDynamic(this, &UMC2SalvageWidget::HandleConfirm);
	if (SkipButton)    SkipButton->OnClicked.AddDynamic(this, &UMC2SalvageWidget::HandleSkip);
}

void UMC2SalvageWidget::SetSalvagePool(const TArray<FName>& ComponentIDs, int32 SalvageBudget)
{
	SelectedComponents.Empty();
	BudgetTotal     = SalvageBudget;
	BudgetRemaining = SalvageBudget;
	RefreshBudgetDisplay();

	// Populate list view — items created as UObject wrappers by BP subclass
	// (SalvageList items set via Blueprint's OnListItemObjectSet)
	if (SalvageList)
	{
		SalvageList->ClearListItems();
		// BP_SalvageItemObject items are added from Blueprint after this call
		// via AddListItem for each ComponentID — see OnSalvagePoolSet event
	}
	OnSalvagePoolSet(ComponentIDs, SalvageBudget);
}

bool UMC2SalvageWidget::ToggleSelectComponent(FName ComponentID, int32 ComponentSalvageCost)
{
	if (SelectedComponents.Contains(ComponentID))
	{
		SelectedComponents.Remove(ComponentID);
		BudgetRemaining += ComponentSalvageCost;
		RefreshBudgetDisplay();
		return true;
	}

	if (BudgetRemaining < ComponentSalvageCost)
	{
		return false;  // not enough budget
	}

	SelectedComponents.Add(ComponentID);
	BudgetRemaining -= ComponentSalvageCost;
	RefreshBudgetDisplay();
	return true;
}

void UMC2SalvageWidget::ConfirmSalvage()
{
	if (UMC2LogisticsSubsystem* LS = GetGameInstance()->GetSubsystem<UMC2LogisticsSubsystem>())
	{
		LS->BeginSalvageSession(BudgetTotal);
		for (const FName& ID : SelectedComponents)
		{
			LS->AcceptSalvageComponent(ID);
		}
		LS->CommitSalvage();
	}
	OnConfirmed();
}

void UMC2SalvageWidget::RefreshBudgetDisplay()
{
	if (BudgetText)
	{
		BudgetText->SetText(FText::FromString(
			FString::Printf(TEXT("Salvage Points: %d / %d"), BudgetTotal - BudgetRemaining, BudgetTotal)));
	}
	if (BudgetBar && BudgetTotal > 0)
	{
		BudgetBar->SetPercent(1.f - (float)BudgetRemaining / (float)BudgetTotal);
	}
}

void UMC2SalvageWidget::HandleConfirm() { ConfirmSalvage(); }
void UMC2SalvageWidget::HandleSkip()
{
	// Skip salvage entirely — just close
	if (UMC2LogisticsSubsystem* LS = GetGameInstance()->GetSubsystem<UMC2LogisticsSubsystem>())
	{
		LS->BeginSalvageSession(0);
		LS->CommitSalvage();
	}
	OnConfirmed();
}
