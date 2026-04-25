#include "Campaign/MC2LogisticsSubsystem.h"
#include "Campaign/MC2MechLabValidator.h"
#include "MC2DataTableRows.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"

void UMC2LogisticsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UMC2LogisticsSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// Save / Load
// ---------------------------------------------------------------------------

FString UMC2LogisticsSubsystem::SlotName(int32 SlotIndex)
{
	return FString::Printf(TEXT("MC2_Save_%d"), SlotIndex);
}

void UMC2LogisticsSubsystem::SaveGame(int32 SlotIndex)
{
	if (!CurrentSave)
		return;

	CurrentSave->SaveTimestamp = FDateTime::Now();
	UGameplayStatics::SaveGameToSlot(CurrentSave, SlotName(SlotIndex), USER_INDEX);
}

void UMC2LogisticsSubsystem::LoadGame(int32 SlotIndex)
{
	USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName(SlotIndex), USER_INDEX);
	CurrentSave = Cast<UMC2SaveGame>(Loaded);
	if (!CurrentSave)
	{
		UE_LOG(LogTemp, Warning, TEXT("MC2LogisticsSubsystem: no save in slot %d, creating new."), SlotIndex);
		CurrentSave = Cast<UMC2SaveGame>(
			UGameplayStatics::CreateSaveGameObject(UMC2SaveGame::StaticClass()));
	}
	OnSaveGameLoaded.Broadcast(CurrentSave);
}

void UMC2LogisticsSubsystem::NewCampaign(const FString& CommanderName, int32 SlotIndex)
{
	CurrentSave = Cast<UMC2SaveGame>(
		UGameplayStatics::CreateSaveGameObject(UMC2SaveGame::StaticClass()));
	CurrentSave->CommanderName          = CommanderName;
	CurrentSave->CurrentOperationIndex  = 0;
	CurrentSave->CurrentMissionIndex    = 0;
	CurrentSave->CBills                 = 1000000;
	CurrentSave->ResourcePoints         = 0;

	SaveGame(SlotIndex);
	OnSaveGameLoaded.Broadcast(CurrentSave);
}

bool UMC2LogisticsSubsystem::HasSaveInSlot(int32 SlotIndex) const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName(SlotIndex), USER_INDEX);
}

// ---------------------------------------------------------------------------
// C-Bills
// ---------------------------------------------------------------------------

bool UMC2LogisticsSubsystem::SpendCBills(int32 Amount)
{
	if (!CurrentSave || CurrentSave->CBills < Amount)
		return false;

	int32 Old = CurrentSave->CBills;
	CurrentSave->CBills -= Amount;
	OnCBillsChanged.Broadcast(Old, CurrentSave->CBills);
	return true;
}

void UMC2LogisticsSubsystem::AddCBills(int32 Amount)
{
	if (!CurrentSave) return;
	int32 Old = CurrentSave->CBills;
	CurrentSave->CBills += Amount;
	OnCBillsChanged.Broadcast(Old, CurrentSave->CBills);
}

int32 UMC2LogisticsSubsystem::GetCBills() const
{
	return CurrentSave ? CurrentSave->CBills : 0;
}

// ---------------------------------------------------------------------------
// Mech roster
// ---------------------------------------------------------------------------

void UMC2LogisticsSubsystem::AddMechToRoster(const FMC2MechRecord& MechRecord)
{
	if (CurrentSave)
		CurrentSave->MechRoster.Add(MechRecord);
}

bool UMC2LogisticsSubsystem::RemoveMechFromRoster(int32 RosterIndex)
{
	if (!CurrentSave || !CurrentSave->MechRoster.IsValidIndex(RosterIndex))
		return false;
	CurrentSave->MechRoster.RemoveAt(RosterIndex);
	return true;
}

const TArray<FMC2MechRecord>& UMC2LogisticsSubsystem::GetMechRoster() const
{
	static TArray<FMC2MechRecord> Empty;
	return CurrentSave ? CurrentSave->MechRoster : Empty;
}

bool UMC2LogisticsSubsystem::AssignPilotToMech(int32 MechRosterIndex, FName PilotID)
{
	if (!CurrentSave || !CurrentSave->MechRoster.IsValidIndex(MechRosterIndex))
		return false;

	// Pilot must be alive
	const FMC2PilotRecord PRecord = GetPilotRecord(PilotID);
	if (!PRecord.bAlive)
		return false;

	// Clear any existing assignment for this pilot before reassigning
	UnassignPilot(PilotID);

	CurrentSave->MechRoster[MechRosterIndex].AssignedPilotID = PilotID;
	return true;
}

void UMC2LogisticsSubsystem::UnassignPilot(FName PilotID)
{
	if (!CurrentSave) return;
	for (FMC2MechRecord& Mech : CurrentSave->MechRoster)
	{
		if (Mech.AssignedPilotID == PilotID)
		{
			Mech.AssignedPilotID = NAME_None;
			return;
		}
	}
}

int32 UMC2LogisticsSubsystem::GetMechIndexForPilot(FName PilotID) const
{
	if (!CurrentSave) return -1;
	for (int32 i = 0; i < CurrentSave->MechRoster.Num(); ++i)
	{
		if (CurrentSave->MechRoster[i].AssignedPilotID == PilotID)
			return i;
	}
	return -1;
}

// ---------------------------------------------------------------------------
// Pilot roster
// ---------------------------------------------------------------------------

void UMC2LogisticsSubsystem::AddPilotToRoster(const FMC2PilotRecord& PilotRecord)
{
	if (CurrentSave)
		CurrentSave->PilotRoster.Add(PilotRecord);
}

void UMC2LogisticsSubsystem::AwardPilotXP(FName PilotID, int32 XP)
{
	if (!CurrentSave) return;
	for (FMC2PilotRecord& Record : CurrentSave->PilotRoster)
	{
		if (Record.PilotID != PilotID || !Record.bAlive)
			continue;

		const int32 OldXP = Record.ExperiencePoints;
		Record.ExperiencePoints += XP;

		// Skill improvement: every 500 XP crossed improves a skill by 1.
		// Gunnery and Piloting improve alternately; minimum skill value is 2.
		// (Threshold: Green 5/5, Regular 4/5, Veteran 3/4, Elite 2/3)
		static const int32 XP_PER_IMPROVEMENT = 500;
		const int32 OldTier = OldXP / XP_PER_IMPROVEMENT;
		const int32 NewTier = Record.ExperiencePoints / XP_PER_IMPROVEMENT;

		for (int32 Tier = OldTier + 1; Tier <= NewTier; ++Tier)
		{
			// Alternate: even tiers improve Gunnery, odd tiers improve Piloting
			if (Tier % 2 == 0)
				Record.Gunnery  = FMath::Max(2, Record.Gunnery  - 1);
			else
				Record.Piloting = FMath::Max(2, Record.Piloting - 1);
		}
		return;
	}
}

void UMC2LogisticsSubsystem::KillPilot(FName PilotID)
{
	if (!CurrentSave) return;
	for (FMC2PilotRecord& Record : CurrentSave->PilotRoster)
	{
		if (Record.PilotID == PilotID)
		{
			Record.bAlive = false;
			return;
		}
	}
}

FMC2PilotRecord UMC2LogisticsSubsystem::GetPilotRecord(FName PilotID) const
{
	if (CurrentSave)
	{
		for (const FMC2PilotRecord& Record : CurrentSave->PilotRoster)
			if (Record.PilotID == PilotID)
				return Record;
	}
	return FMC2PilotRecord{};
}

// ---------------------------------------------------------------------------
// Mission progression
// ---------------------------------------------------------------------------

void UMC2LogisticsSubsystem::RecordMissionResult(FName MissionID, uint8 Result, int32 CBillReward)
{
	if (!CurrentSave) return;

	FMC2SavedMissionResult Entry;
	Entry.MissionID     = MissionID;
	Entry.ResultValue   = Result;
	Entry.CBillsEarned  = CBillReward;
	CurrentSave->CompletedMissions.Add(Entry);

	AddCBills(CBillReward);
}

void UMC2LogisticsSubsystem::AdvanceToNextMission()
{
	if (!CurrentSave) return;
	CurrentSave->CurrentMissionIndex++;
	// Operation advancement is handled by the campaign data asset / Blueprint
}

bool UMC2LogisticsSubsystem::IsMissionComplete(FName MissionID) const
{
	if (!CurrentSave) return false;
	for (const FMC2SavedMissionResult& R : CurrentSave->CompletedMissions)
		if (R.MissionID == MissionID)
			return true;
	return false;
}

// ---------------------------------------------------------------------------
// Campaign flags
// ---------------------------------------------------------------------------

void UMC2LogisticsSubsystem::SetCampaignFlag(int32 FlagIndex, bool Value)
{
	if (!CurrentSave || !CurrentSave->CampaignFlags.IsValidIndex(FlagIndex))
		return;
	CurrentSave->CampaignFlags[FlagIndex] = Value;
}

bool UMC2LogisticsSubsystem::GetCampaignFlag(int32 FlagIndex) const
{
	if (!CurrentSave || !CurrentSave->CampaignFlags.IsValidIndex(FlagIndex))
		return false;
	return CurrentSave->CampaignFlags[FlagIndex];
}

// ---------------------------------------------------------------------------
// Salvage system
// ---------------------------------------------------------------------------

void UMC2LogisticsSubsystem::BeginSalvageSession(const TArray<FName>& AvailableComponents, int32 SalvagePoints)
{
	PendingSalvagePool      = AvailableComponents;
	AcceptedSalvage.Reset();
	SalvageBudgetTotal      = SalvagePoints;
	SalvageBudgetRemaining  = SalvagePoints;
}

bool UMC2LogisticsSubsystem::AcceptSalvageComponent(FName ComponentID)
{
	if (!PendingSalvagePool.Contains(ComponentID))
		return false;

	const int32 Cost = GetComponentBR(ComponentID);
	if (Cost > SalvageBudgetRemaining)
		return false;

	AcceptedSalvage.Add(ComponentID);
	PendingSalvagePool.Remove(ComponentID);
	SalvageBudgetRemaining -= Cost;
	return true;
}

void UMC2LogisticsSubsystem::CommitSalvage()
{
	if (!CurrentSave)
		return;

	// Add each salvaged component to the player's component inventory
	for (const FName& CompID : AcceptedSalvage)
		CurrentSave->ComponentInventory.Add(CompID);

	AcceptedSalvage.Reset();
	PendingSalvagePool.Reset();
}

void UMC2LogisticsSubsystem::CancelSalvage()
{
	AcceptedSalvage.Reset();
	PendingSalvagePool.Reset();
}

int32 UMC2LogisticsSubsystem::GetComponentBR(FName ComponentID) const
{
	if (!ComponentTable)
		return 10;  // default cost if table not set

	if (FMC2ComponentRow* Row = ComponentTable->FindRow<FMC2ComponentRow>(ComponentID, TEXT("Salvage")))
		return FMath::Max(1, Row->BattleRating);

	return 10;
}

// ---------------------------------------------------------------------------
// Repair system
// ---------------------------------------------------------------------------

int32 UMC2LogisticsSubsystem::GetRepairCost(int32 RosterIndex, UDataTable* ChassisTable) const
{
	if (!CurrentSave || !CurrentSave->MechRoster.IsValidIndex(RosterIndex) || !ChassisTable)
		return 0;

	const FMC2MechRecord& Mech = CurrentSave->MechRoster[RosterIndex];
	FMC2MechChassisRow* Chassis = ChassisTable->FindRow<FMC2MechChassisRow>(Mech.ChassisID, TEXT("RepairCost"));
	if (!Chassis)
		return 0;

	// Total armor damage = (max armor - current armor) summed across all zones
	int32 TotalMaxArmor = Chassis->Armor_Head + Chassis->Armor_LA + Chassis->Armor_RA +
	                      Chassis->Armor_LT + Chassis->Armor_RT + Chassis->Armor_CT +
	                      Chassis->Armor_LL + Chassis->Armor_RL +
	                      Chassis->Armor_RearLT + Chassis->Armor_RearRT + Chassis->Armor_RearCT;

	int32 CurrentArmor = 0;
	for (int32 i = 0; i < Mech.CurrentArmor.Num(); ++i)
		CurrentArmor += Mech.CurrentArmor[i];

	const int32 Damage = FMath::Max(0, TotalMaxArmor - CurrentArmor);
	return Damage * RepairCostPerArmor;
}

bool UMC2LogisticsSubsystem::RepairMech(int32 RosterIndex, UDataTable* ChassisTable)
{
	const int32 Cost = GetRepairCost(RosterIndex, ChassisTable);
	if (Cost <= 0)
		return true;  // already fully repaired

	if (!SpendCBills(Cost))
		return false;  // insufficient funds

	// Reset all armor zones to max (full repair)
	if (!CurrentSave || !CurrentSave->MechRoster.IsValidIndex(RosterIndex) || !ChassisTable)
		return false;

	FMC2MechRecord& Mech = CurrentSave->MechRoster[RosterIndex];
	FMC2MechChassisRow* Chassis = ChassisTable->FindRow<FMC2MechChassisRow>(Mech.ChassisID, TEXT("Repair"));
	if (!Chassis)
		return false;

	TArray<int32> MaxArmors = {
		Chassis->Armor_Head, Chassis->Armor_LA,  Chassis->Armor_RA,
		Chassis->Armor_LT,   Chassis->Armor_RT,  Chassis->Armor_CT,
		Chassis->Armor_LL,   Chassis->Armor_RL,
		Chassis->Armor_RearLT, Chassis->Armor_RearRT, Chassis->Armor_RearCT
	};

	Mech.CurrentArmor = MaxArmors;
	return true;
}

// ---------------------------------------------------------------------------
// Variant system
// ---------------------------------------------------------------------------

FMC2MechRecord UMC2LogisticsSubsystem::LoadFactoryVariant(FName ChassisID, int32 VariantIndex) const
{
	FMC2MechRecord Record;
	Record.ChassisID    = ChassisID;
	Record.VariantIndex = VariantIndex;

	if (!MechVariantTable)
		return Record;

	// Row key convention: ChassisID_VariantIndex (e.g., "Atlas_0", "Atlas_1")
	FString RowKey = FString::Printf(TEXT("%s_%d"), *ChassisID.ToString(), VariantIndex);
	FMC2MechVariantRow* VRow = MechVariantTable->FindRow<FMC2MechVariantRow>(FName(RowKey), TEXT("LoadVariant"));
	if (!VRow)
		return Record;

	// Convert Item0-Item19 integers to FName component IDs
	Record.ComponentSlots.Reset();
	auto AddSlot = [&](int32 ItemID) {
		Record.ComponentSlots.Add(ItemID > 0 ? FName(*FString::FromInt(ItemID)) : NAME_None);
	};
	AddSlot(VRow->Item0);  AddSlot(VRow->Item1);  AddSlot(VRow->Item2);  AddSlot(VRow->Item3);
	AddSlot(VRow->Item4);  AddSlot(VRow->Item5);  AddSlot(VRow->Item6);  AddSlot(VRow->Item7);
	AddSlot(VRow->Item8);  AddSlot(VRow->Item9);  AddSlot(VRow->Item10); AddSlot(VRow->Item11);
	AddSlot(VRow->Item12); AddSlot(VRow->Item13); AddSlot(VRow->Item14); AddSlot(VRow->Item15);
	AddSlot(VRow->Item16); AddSlot(VRow->Item17); AddSlot(VRow->Item18); AddSlot(VRow->Item19);

	return Record;
}

bool UMC2LogisticsSubsystem::SaveCustomVariant(int32 RosterIndex, const TArray<FName>& NewComponentSlots)
{
	if (!CurrentSave || !CurrentSave->MechRoster.IsValidIndex(RosterIndex))
		return false;

	CurrentSave->MechRoster[RosterIndex].ComponentSlots = NewComponentSlots;
	return true;
}

bool UMC2LogisticsSubsystem::ResetToFactoryVariant(int32 RosterIndex)
{
	if (!CurrentSave || !CurrentSave->MechRoster.IsValidIndex(RosterIndex))
		return false;

	const FMC2MechRecord& Existing = CurrentSave->MechRoster[RosterIndex];
	FMC2MechRecord Factory = LoadFactoryVariant(Existing.ChassisID, Existing.VariantIndex);
	CurrentSave->MechRoster[RosterIndex].ComponentSlots = Factory.ComponentSlots;
	return true;
}
