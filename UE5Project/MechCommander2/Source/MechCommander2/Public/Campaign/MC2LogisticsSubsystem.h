#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Campaign/MC2SaveGame.h"
#include "MC2LogisticsSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveGameLoaded, UMC2SaveGame*, SaveGame);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCBillsChanged, int32, OldAmount, int32, NewAmount);

/**
 * UMC2LogisticsSubsystem
 * GameInstance subsystem — persistent across level loads.
 * Owns the current save game and exposes the campaign API used by
 * logistics screens and mission debrief.
 *
 * Maps to MC2's logisticsData global class (logistics.h / logistics.cpp).
 *
 * Access: UGameInstance::GetSubsystem<UMC2LogisticsSubsystem>()
 * Or BP:  GetGameInstance → Get Subsystem (MC2LogisticsSubsystem)
 */
UCLASS()
class MECHCOMMANDER2_API UMC2LogisticsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// --- Save / Load ---

	UFUNCTION(BlueprintCallable, Category = "Campaign|Save")
	void SaveGame(int32 SlotIndex = 0);

	UFUNCTION(BlueprintCallable, Category = "Campaign|Save")
	void LoadGame(int32 SlotIndex = 0);

	UFUNCTION(BlueprintCallable, Category = "Campaign|Save")
	void NewCampaign(const FString& CommanderName, int32 SlotIndex = 0);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Campaign|Save")
	UMC2SaveGame* GetCurrentSave() const { return CurrentSave; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Campaign|Save")
	bool HasSaveInSlot(int32 SlotIndex) const;

	// --- C-Bills ---

	UFUNCTION(BlueprintCallable, Category = "Campaign|Economy")
	bool SpendCBills(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Campaign|Economy")
	void AddCBills(int32 Amount);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Campaign|Economy")
	int32 GetCBills() const;

	// --- Mech roster ---

	UFUNCTION(BlueprintCallable, Category = "Campaign|Roster")
	void AddMechToRoster(const FMC2MechRecord& MechRecord);

	UFUNCTION(BlueprintCallable, Category = "Campaign|Roster")
	bool RemoveMechFromRoster(int32 RosterIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Campaign|Roster")
	const TArray<FMC2MechRecord>& GetMechRoster() const;

	// Assign a pilot to a mech slot. Fails if the pilot is dead or already
	// assigned to another mech. Clears their previous assignment first.
	UFUNCTION(BlueprintCallable, Category = "Campaign|Roster")
	bool AssignPilotToMech(int32 MechRosterIndex, FName PilotID);

	// Remove a pilot from whatever mech they are currently assigned to.
	UFUNCTION(BlueprintCallable, Category = "Campaign|Roster")
	void UnassignPilot(FName PilotID);

	// Get the roster index of the mech the pilot is currently assigned to (-1 = none).
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Campaign|Roster")
	int32 GetMechIndexForPilot(FName PilotID) const;

	// --- Pilot roster ---

	UFUNCTION(BlueprintCallable, Category = "Campaign|Pilots")
	void AddPilotToRoster(const FMC2PilotRecord& PilotRecord);

	UFUNCTION(BlueprintCallable, Category = "Campaign|Pilots")
	void AwardPilotXP(FName PilotID, int32 XP);

	UFUNCTION(BlueprintCallable, Category = "Campaign|Pilots")
	void KillPilot(FName PilotID);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Campaign|Pilots")
	FMC2PilotRecord GetPilotRecord(FName PilotID) const;

	// --- Salvage system ---
	// After a mission the server calculates salvageable units/components.
	// Player selects which to keep up to the salvage point budget.

	// Begin a salvage session: populate available components from destroyed enemies.
	// SalvagePoints = budget; each component costs its BattleRating in points.
	UFUNCTION(BlueprintCallable, Category = "Campaign|Salvage")
	void BeginSalvageSession(const TArray<FName>& AvailableComponents, int32 SalvagePoints);

	// Accept a component into inventory during salvage. Returns false if over budget.
	UFUNCTION(BlueprintCallable, Category = "Campaign|Salvage")
	bool AcceptSalvageComponent(FName ComponentID);

	// Finalize salvage: commit accepted components to save, end the session.
	UFUNCTION(BlueprintCallable, Category = "Campaign|Salvage")
	void CommitSalvage();

	// Cancel salvage (player skips screen).
	UFUNCTION(BlueprintCallable, Category = "Campaign|Salvage")
	void CancelSalvage();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Campaign|Salvage")
	const TArray<FName>& GetAvailableSalvage() const { return PendingSalvagePool; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Campaign|Salvage")
	const TArray<FName>& GetAcceptedSalvage() const { return AcceptedSalvage; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Campaign|Salvage")
	int32 GetRemainingBudget() const { return SalvageBudgetRemaining; }

	// --- Repair system ---
	// Calculates repair cost for a mech and deducts C-Bills.
	// Cost = total armor damage × RepairCostPerArmor (default 500 C-Bills).
	UFUNCTION(BlueprintCallable, Category = "Campaign|Repair")
	bool RepairMech(int32 RosterIndex, UDataTable* ChassisTable);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Campaign|Repair")
	int32 GetRepairCost(int32 RosterIndex, UDataTable* ChassisTable) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Campaign|Repair")
	int32 RepairCostPerArmor = 500;  // C-Bills per armor point repaired

	// --- Variant system ---
	// Factory variants come from DT_MechVariants (read-only, set in editor).
	// Custom variants are player-created loadouts stored in the save game.

	// Reference to the mech variant Data Table (set in editor via GameInstance defaults)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign|Variants")
	TObjectPtr<class UDataTable> MechVariantTable;

	// Reference to the component Data Table (for weight/slot lookups during editing)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign|Variants")
	TObjectPtr<class UDataTable> ComponentTable;

	// Load a factory variant loadout from DT_MechVariants into a MechRecord.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Campaign|Variants")
	FMC2MechRecord LoadFactoryVariant(FName ChassisID, int32 VariantIndex) const;

	// Save a custom player variant back to the roster slot (overwrites ComponentSlots).
	UFUNCTION(BlueprintCallable, Category = "Campaign|Variants")
	bool SaveCustomVariant(int32 RosterIndex, const TArray<FName>& NewComponentSlots);

	// Reset a mech at RosterIndex back to its factory default variant.
	UFUNCTION(BlueprintCallable, Category = "Campaign|Variants")
	bool ResetToFactoryVariant(int32 RosterIndex);

	// --- Mission progression ---

	UFUNCTION(BlueprintCallable, Category = "Campaign|Mission")
	void RecordMissionResult(FName MissionID, uint8 Result, int32 CBillReward);

	UFUNCTION(BlueprintCallable, Category = "Campaign|Mission")
	void AdvanceToNextMission();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Campaign|Mission")
	bool IsMissionComplete(FName MissionID) const;

	// --- Campaign flags (ABL eternal booleans shared across missions) ---

	UFUNCTION(BlueprintCallable, Category = "Campaign|Flags")
	void SetCampaignFlag(int32 FlagIndex, bool Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Campaign|Flags")
	bool GetCampaignFlag(int32 FlagIndex) const;

	// --- Tech base ---
	// Clan-tech components become available when CampaignFlag[32] is set.
	// This flag is written by BP_MC2Mission_13 (the Clan introduction mission).
	// Index 32 is reserved for this purpose; ABL eternal var "clanTechAvailable".

	UFUNCTION(BlueprintCallable, Category = "Campaign|TechBase")
	void UnlockClanTech() { SetCampaignFlag(ClanTechFlagIndex, true); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Campaign|TechBase")
	bool IsClanTechUnlocked() const { return GetCampaignFlag(ClanTechFlagIndex); }

	// --- Events ---

	UPROPERTY(BlueprintAssignable, Category = "Campaign|Events")
	FOnSaveGameLoaded OnSaveGameLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Campaign|Events")
	FOnCBillsChanged OnCBillsChanged;

	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	UPROPERTY()
	TObjectPtr<UMC2SaveGame> CurrentSave;

	// Salvage session state
	TArray<FName> PendingSalvagePool;
	TArray<FName> AcceptedSalvage;
	int32 SalvageBudgetTotal = 0;
	int32 SalvageBudgetRemaining = 0;

	// Helper: look up component BattleRating (salvage cost)
	int32 GetComponentBR(FName ComponentID) const;

	static FString SlotName(int32 SlotIndex);
	static constexpr int32 USER_INDEX = 0;

	// CampaignFlags index reserved for the Clan tech unlock (set by mission 13).
	static constexpr int32 ClanTechFlagIndex = 32;
};
