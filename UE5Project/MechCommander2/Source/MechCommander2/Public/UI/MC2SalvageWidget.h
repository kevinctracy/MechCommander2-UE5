#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MC2SalvageWidget.generated.h"

class UListView;
class UTextBlock;
class UButton;
class UProgressBar;

/**
 * UMC2SalvageWidget  (WBP_Salvage)
 * Post-mission salvage selection screen.
 * Player picks components from destroyed enemy units up to a salvage point budget.
 *
 * Salvage budget = function of mission result + bonus objectives.
 * Components are listed in a scrollable grid; drag to inventory or click to toggle select.
 */
UCLASS(Abstract, BlueprintType)
class MECHCOMMANDER2_API UMC2SalvageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Populate with available salvage items (ComponentIDs from destroyed enemies)
	UFUNCTION(BlueprintCallable, Category = "Salvage")
	void SetSalvagePool(const TArray<FName>& ComponentIDs, int32 SalvageBudget);

	// Toggle selection of a component; returns false if budget exceeded
	UFUNCTION(BlueprintCallable, Category = "Salvage")
	bool ToggleSelectComponent(FName ComponentID, int32 ComponentSalvageCost);

	// Confirm selection and add to logistics inventory
	UFUNCTION(BlueprintCallable, Category = "Salvage")
	void ConfirmSalvage();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Salvage")
	int32 GetRemainingBudget() const { return BudgetRemaining; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Salvage")
	void OnConfirmed();

	// Called after SetSalvagePool so Blueprint can populate ListView items
	UFUNCTION(BlueprintImplementableEvent, Category = "Salvage")
	void OnSalvagePoolSet(const TArray<FName>& ComponentIDs, int32 Budget);

protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UListView>    SalvageList;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock>   BudgetText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> BudgetBar;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>      ConfirmButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>      SkipButton;

	virtual void NativeConstruct() override;

	UFUNCTION() void HandleConfirm();
	UFUNCTION() void HandleSkip();

private:
	int32 BudgetTotal     = 0;
	int32 BudgetRemaining = 0;
	TArray<FName> SelectedComponents;

	void RefreshBudgetDisplay();
};
