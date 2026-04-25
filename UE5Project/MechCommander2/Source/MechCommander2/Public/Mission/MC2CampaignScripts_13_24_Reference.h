/**
 * MC2CampaignScripts_13_24_Reference.h
 *
 * ABL → UE5 Blueprint translation for mc2_13.abl through mc2_24.abl.
 * This covers Acts 2-3 of the MechCommander 2 campaign.
 *
 * Read MC2MissionScript_Reference.h for the shared ABL function table.
 * Read MC2CampaignScripts_02_12_Reference.h for the eternal flag convention
 * (MissionFlags[30+] = mission-unique triggers) and warrior polling pattern.
 *
 * IMPORTANT MODULE NAME DISCREPANCIES
 * ------------------------------------
 *  mc2_21.abl  → module mc2_11       (reuses mc2_11 sensor-net script)
 *  mc2_22.abl  → module mc2_liaoPalace (custom name, unique script)
 *  mc2_23.abl  → module mc2_07       (reuses mc2_07 patrol script structure)
 *
 *  For these three, implement distinct BP_MC2Mission_21/22/23 classes even
 *  though the ABL module is named differently — only the flag indices matter.
 *
 * ===========================================================================
 * MISSION 13 — "Operation: Hammer of God" (HPG facility)
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune6 → Content/Audio/Music/S_MissionStart_06
 *
 *  Mission-unique MissionFlags (30+):
 *    30  MainNorthAttacked    — HPG northern units are being attacked
 *    31  MainSouthAttacked    — HPG southern units are being attacked
 *    32  WeapFac1Attacked     — Weapons Factory 1 under attack
 *    33  WeapFac2Attacked     — Weapons Factory 2 under attack
 *    34  HPGAttacked          — either HPG is under attack
 *    35  ClanBeenAttack       — Clan units engaged
 *
 *  Trigger areas (integer IDs → ATriggerVolume):
 *    clanAreaTrigger          — area near Clan deployment zone
 *
 *  Part references (integer IDs → TArray<AMC2Mover*> on GameMode):
 *    outerPatrol_1_ID         — lead unit of outer patrol group 1
 *
 *  VO sequence:
 *    HPGAttacked → PlayVO("S_mc2_13_3")   // Son attacks player VO
 *    Clan event  → PlayVO("S_mc2_13_2")
 *
 *  Videos: "MC2_13B", "W09", "W06", "W12"
 *  (W-prefix = C-Bill reward videos; MC2-prefix = story cutscenes)
 *
 *  Objective gating:
 *    HPG objectives tie into weapon factory destruction flags.
 *    Both HPGAttacked=true when either HPG objective is incomplete and player
 *    is detected near either structure.
 *
 * ===========================================================================
 * MISSION 14 — "Operation: Grim Reaper" (Wall breach / patrol ambush)
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune8
 *
 *  Mission-unique MissionFlags (30+):
 *    30  patrol1_triggered
 *    31  patrol2_triggered
 *    32  patrol3_triggered
 *    33  patrols_Area_Trigger  — player entered patrol zone
 *
 *  VO:
 *    patrols_Area_Trigger → PlayVO("S_mc2_14_0")  // "Wall breached" tac officer
 *
 *  Videos: "MC2_14C", "MC2_14D"
 *
 * ===========================================================================
 * MISSION 15 — "Operation: Rebel Yell" (TCB escort, rebel support)
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune7
 *
 *  Mission-unique MissionFlags (30+):
 *    30  TurretBase_Attacked   — turret control base under attack
 *    31  SupplyBase_Attacked   — supply base under attack
 *    32  LegionAttacked        — Legion Tank has been engaged
 *    33  AttackGrid            — player/rebels attacking grid
 *    34  RebelAliveCheck       — rebel unit still alive
 *    35  OperativeVo           — operative VO has played
 *    36  banditRunAway         — bandit group is fleeing
 *
 *  Part references (integer IDs):
 *    TCB_Escort_1, TCB_Escort_2  — escort unit Part IDs for TCB mission
 *
 *  Trigger areas:
 *    TCB_AreaTriggered           — TCB zone entered
 *
 *  VO sequence (multiple):
 *    RebelAliveCheck → PlayVO("S_mc2_15_0")   // Rebel calling for help
 *    LegionAttacked → PlayVO("S_mc2_15_5")    // Steiner talks about Legion Tank
 *    LegionAttacked → PlayVO("S_mc2_15_3")    // Steiner cont.
 *    AttackGrid → PlayVO("S_mc2_15_6")        // Rebel calls again
 *    banditRunAway → PlayVO("S_mc2_15_4")     // Rebel blows up sensor
 *
 *  No video cutscenes.
 *
 * ===========================================================================
 * MISSION 16 — "Operation: Hammer of Iron" (Multi-wave assault)
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune2
 *
 *  Mission-unique MissionFlags (30+):
 *    30  FirstEast_Start_Trigger
 *    31  FirstEast2_Start_Trigger
 *    32  FirstMain_Start_Trigger
 *    33  SecondEast_Start_Trigger
 *    34  SecondEast2_Start_Trigger
 *    35  SecondMain_Start_Trigger
 *    36  SecondMain2_Start_Trigger
 *    37  ThirdEast_Start_Trigger
 *    38  ThirdEast2_Start_Trigger
 *    39  ThirdMain_Start_Trigger
 *    40  FourthMain_Start_Trigger
 *
 *  These are pure timing-gate triggers set by the main script to order
 *  successive waves into combat. Warrior scripts poll each flag and begin
 *  moving only when their assigned flag is true.
 *
 *  No VO, no videos. Pure combat wave management.
 *
 *  UE5: All 11 flags driven by objective completion + FTimerHandle delays.
 *  Wire each flag via OnObjectiveComplete(N) → SetMissionFlag(30+M, true).
 *
 * ===========================================================================
 * MISSION 17 — "Operation: Dragon's Blood" (Liao betrayal, key story mission)
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune11 → Content/Audio/Music/S_MissionStart_11
 *  (Many cinematic camera sweeps — most complex mission script in the campaign)
 *
 *  Mission-unique MissionFlags (30+):
 *    30  LiaoSonMoveToValley   — Liao Son's units move to the valley
 *    31  SteinerMoveToValley   — Steiner units move to valley
 *    32  BothRunAways          — both Steiner and Liao flee
 *    33  liaoBetrays           — Steiner units attack player (betrayal trigger)
 *    34  LiaoSonAttack         — Liao Son's units begin attacking
 *    35  LiaoSendReinforce     — reinforcements called in
 *    36  LiaoReinforce_1_Left  — tracks left flank movement
 *    37  LiaoReinforce_1_Right — tracks right flank movement
 *
 *  Camera sequence (Level Sequencer in UE5):
 *    eternal real[3] camRotation / camPosition / startCameraPosition / startCameraRotation
 *    → 5-stage flythrough on mission start (movie mode)
 *    UE5: LevelSequence/Cutscenes/LS_MC2_17_Intro with camera cuts
 *
 *  Part references:
 *    escortUnitID  — Liao Son's escort unit Part ID; warrior scripts guard him
 *
 *  VO sequence (key betrayal dialogue):
 *    liaoBetrays trigger → PlayVO("S_mc2_17_8")   // Son: "Do nothing"
 *    Next → PlayVO("S_mc2_17_9")                   // Son: "Open channel"
 *    Next → PlayVO("S_mc2_17_a")                   // Renard: "Good day"
 *    Next → PlayVO("S_mc2_17_b")                   // Son: impressed
 *    Next → PlayVO("S_mc2_17_C")                   // Renard: respect
 *
 *  Videos: "MC2_17A", "MC2_17B", "MC2_17C", "MC2_17D", "MC2_17E"
 *  (5 cutscenes — the most of any mission; play them in sequence via LevelSequencePlayer)
 *
 * ===========================================================================
 * MISSION 18 — "Operation: Rubicon" (Group-wave assault, camera intro)
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune3 → Content/Audio/Music/S_MissionStart_03
 *
 *  Mission-unique MissionFlags (30+):
 *    30  goGroup0  — group 0 begins attack
 *    31  goGroup1
 *    32  goGroup2
 *    33  goGroup3
 *    34  goGroup4
 *    35  goSneaky  — "sneaky" flanking group activates
 *
 *  Camera sequence: eternal real[3] camRotation/camPosition/camStartingPos/camStartingRot
 *    → cinematic intro pan, same pattern as mc2_10 and mc2_17
 *    UE5: LS_MC2_18_Intro LevelSequence
 *
 *  VO:
 *    checkObjectiveStatus(building attacked) → PlayVO("S_mc2_18_1")
 *      // "Objective building is under attack"
 *    Secondary → PlayVO("S_mc2_18_0")
 *
 *  No video cutscenes.
 *
 * ===========================================================================
 * MISSION 19 — "Operation: Thunderstrike" (Airport + port defense)
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune8
 *
 *  Mission-unique MissionFlags (30+):
 *    30  IndustryAttacked  — reinforcements sent to industry area
 *    31  PortAttacked      — reinforcements sent to airport/port area
 *    32  HangerAttacked    — reinforcements sent to hangar area
 *
 *  Timer:
 *    oneThirdEnemy — real timer used to play "1/3 of enemies destroyed" VO
 *    (tracks when 33% of enemy force has been killed)
 *
 *  VO:
 *    oneThirdEnemy expires → PlayVO("S_mc2_19_0")  // Son attacks player VO
 *
 *  Video: "W07" (C-Bill reward)
 *
 * ===========================================================================
 * MISSION 20 — "Operation: Last Resort" (Mog facility, prison ambush)
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune4 → Content/Audio/Music/S_MissionStart_04
 *
 *  Mission-unique MissionFlags (30+):
 *    30  baseAttacked     — Mog base under attack
 *    31  DshipAttacked    — destroyer/ambush units triggered
 *    32  MogAttacked      — Mog powerdown node attacked
 *    33  PrisionAttack    — prison area attacked
 *
 *  Part references:
 *    MogM_PartID     — integer: moving Mog unit Part ID
 *
 *  World position:
 *    MogM_StartPos   — WorldPosition of moving Mog start location
 *    UE5: Store as FVector MogStartLocation on the GameMode BP.
 *
 *  VO sequence:
 *    MogAttacked → PlayVO("S_mc2_20_0")    // Son attacks player
 *    DshipAttacked → PlayVO("S_mc2_20_1")
 *    PrisionAttack → PlayVO("S_mc2_20_2")
 *
 *  No video cutscenes.
 *
 * ===========================================================================
 * MISSION 21 — (module mc2_11 reused) — Sensor net variant
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune10
 *
 *  NOTE: mc2_21.abl is `module mc2_11`. This mission reuses the sensor-net
 *  script but with a different map and objective layout.
 *
 *  Mission-unique MissionFlags (30+):
 *    30  alphaPG_BeenAreaTriggered  — alpha power grid zone entered
 *    31  betaPG_BeenAreaTriggered   — beta power grid zone entered
 *    32  wallhit                    — player breached wall
 *
 *  Video: "W10" (C-Bill reward)
 *
 *  UE5: BP_MC2Mission_21 is a standalone class (does NOT inherit mc2_11).
 *  The eternal flags differ from mc2_11 (which has SensorVOTrigger, bridges).
 *
 * ===========================================================================
 * MISSION 22 — "Operation: Storm the Palace" (module mc2_liaoPalace)
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune2
 *
 *  Mission-unique MissionFlags (30+):
 *    30  baseUnderAttack    — palace base is under attack
 *    31  palaceBeenAttack   — palace itself has been attacked
 *    32  eliteBeenHit       — elite palace guard engaged
 *    33  perimeterAlarm     — perimeter alarm triggered
 *    34  ForestAttackWest   — west forest attack begun
 *    35  ForestAttackEast
 *
 *  Trigger areas (integer IDs → ATriggerVolume):
 *    WestBridgeTrigger
 *    CenterBridgeTrigger
 *    EastBridgeTrigger
 *
 *  VO:
 *    palaceBeenAttack → PlayVO("S_mc2_22")   // Palace guard speaks
 *
 *  No video cutscenes.
 *
 * ===========================================================================
 * MISSION 23 — (module mc2_07 reused) — Starport / rebel rescue
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune3
 *
 *  NOTE: mc2_23.abl is `module mc2_07`. Much more complex than the original
 *  mc2_07 (sandbox patrol). This mission has extensive unique flags.
 *
 *  Mission-unique MissionFlags (30+):
 *    30  patrols_Area_Trigger
 *    31  Valley_Trap_triggered       — valley ambush activated
 *    32  EC2_PlayerAtBridge          — player reached bridge
 *    33  EC2_PlayerAtWater           — player at water area
 *    34  EC2_Recapture_Sensors       — player recaptured sensors
 *    35  Rebels_Freed                — rebel prisoners freed
 *    36  Starport_Attacked_North     — north starport under attack
 *    37  Starport_Attacked_South
 *    38  North_Drop_Attacked         — north dropship zone attacked
 *    39  South_Drop_Attacked
 *    40  East_Drop_Attacked
 *    41  Starport_Reinforcements_Trigger — reinforcements deployed to starport
 *    42  Southbase_Attacked          — south base under attack
 *    43  Cv_End_Trigger              — convoy reached endpoint
 *
 *  VO sequence (multiple tac officer lines):
 *    Valley_Trap_triggered → PlayVO("S_mc2_23_1")
 *    EC2_PlayerAtBridge   → PlayVO("S_mc2_23_2")    // "Wall breached" style
 *    Rebels_Freed         → PlayVO("S_mc2_23_3")
 *    Starport_Attacked    → PlayVO("S_mc2_23_4")
 *    Reinforcements event → PlayVO("S_mc2_23_5")
 *
 *  No video cutscenes.
 *
 * ===========================================================================
 * MISSION 24 — "Operation: Final Battle" (Renard's last stand, campaign end)
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune6
 *
 *  Mission-unique MissionFlags (30+):
 *    30  Rebels_Attack        — rebel factions attack alongside player
 *    31  Rebel_Mech_Attack    — rebel mechs engage
 *    32  TC1_Captured         — tactical center 1 captured
 *    33  TC2_Captured         — tactical center 2 captured
 *    34  East_Area_Triggered  — player entered east zone
 *    35  Center_Area_Triggered
 *    36  Renard_Attack        — Renard's personal lance begins attacking
 *    37  Powered_Armor_Trigger — power armor units deploy
 *
 *  VO sequence (Renard taunts):
 *    Renard_Attack → PlayVO("S_mc2_24_3")   // Tac officer warning
 *    TC1_Captured  → PlayVO("S_mc2_24_4")   // Tac officer
 *    TC2_Captured  → PlayVO("S_mc2_24_5")   // Renard: "I'm coming for you"
 *    Player near Renard → PlayVO("S_mc2_24_6")  // Renard taunts (var 1)
 *    Player near Renard → PlayVO("S_mc2_24_7")  // Renard taunts (var 2)
 *
 *  Videos (campaign finale — play in sequence on final objective):
 *    "MC2_24A", "MC2_24B", "MC2_24C", "MC2_24D", "MC2_24E"
 *    UE5: Chain 5 LevelSequencePlayer instances or use a single master sequence.
 *
 *  TC1_Captured + TC2_Captured both true → trigger Renard_Attack.
 *  Powered_Armor_Trigger → deploy final wave when Renard is low HP.
 *
 * ===========================================================================
 * COMPLETE MISSION MUSIC TABLE (all 24 missions)
 * ===========================================================================
 *
 *  Mission  StartTune     UE5 Asset
 *  mc2_01   Tune6         S_MissionStart_06
 *  mc2_02   Tune6         S_MissionStart_06
 *  mc2_03   (see mc2_12)  S_MissionStart_05
 *  mc2_04   Tune8         S_MissionStart_08
 *  mc2_05   Tune1         S_MissionStart_01
 *  mc2_06   Tune10        S_MissionStart_10
 *  mc2_07   Tune1         S_MissionStart_01
 *  mc2_08   Tune7         S_MissionStart_07
 *  mc2_09   Tune9         S_MissionStart_09
 *  mc2_10   Tune0         S_MissionStart_00
 *  mc2_11   Tune2         S_MissionStart_02
 *  mc2_12   Tune5         S_MissionStart_05
 *  mc2_13   Tune6         S_MissionStart_06
 *  mc2_14   Tune8         S_MissionStart_08
 *  mc2_15   Tune7         S_MissionStart_07
 *  mc2_16   Tune2         S_MissionStart_02
 *  mc2_17   Tune11        S_MissionStart_11
 *  mc2_18   Tune3         S_MissionStart_03
 *  mc2_19   Tune8         S_MissionStart_08
 *  mc2_20   Tune4         S_MissionStart_04
 *  mc2_21   Tune10        S_MissionStart_10
 *  mc2_22   Tune2         S_MissionStart_02
 *  mc2_23   Tune3         S_MissionStart_03
 *  mc2_24   Tune6         S_MissionStart_06
 *
 * ===========================================================================
 * UE5 BLUEPRINT CLASS HIERARCHY SUMMARY
 * ===========================================================================
 *
 *  AMC2GameMode (C++)
 *    └─ BP_MC2Mission_01  (unique logic)
 *    └─ BP_MC2Mission_02  (unique logic)
 *    └─ BP_MC2Mission_03  (same structure as _12, different level)
 *    └─ BP_MC2Mission_04
 *    └─ BP_MC2Mission_05
 *    └─ BP_MC2Mission_06
 *    └─ BP_MC2Mission_07
 *    └─ BP_MC2Mission_08  (minimal — music only)
 *    └─ BP_MC2Mission_09
 *    └─ BP_MC2Mission_10  (camera sequences)
 *    └─ BP_MC2Mission_11
 *    └─ BP_MC2Mission_12  (base for _03 and _12)
 *    └─ BP_MC2Mission_13
 *    └─ BP_MC2Mission_14
 *    └─ BP_MC2Mission_15
 *    └─ BP_MC2Mission_16  (11 wave trigger flags)
 *    └─ BP_MC2Mission_17  (camera intro + 5 videos — most complex)
 *    └─ BP_MC2Mission_18
 *    └─ BP_MC2Mission_19
 *    └─ BP_MC2Mission_20
 *    └─ BP_MC2Mission_21  (unique despite mc2_11 module name)
 *    └─ BP_MC2Mission_22  (Liao Palace)
 *    └─ BP_MC2Mission_23  (complex, despite mc2_07 module name)
 *    └─ BP_MC2Mission_24  (finale — 5 ending cutscenes)
 *
 *  Each BP class overrides OnMissionStart() and handles mission-unique
 *  flag/VO/timer logic. Warrior BT scripts poll via ScriptGetMissionFlag().
 */

// This file is documentation only — not compiled.
