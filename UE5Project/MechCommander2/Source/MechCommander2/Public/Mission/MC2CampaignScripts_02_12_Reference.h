/**
 * MC2CampaignScripts_02_12_Reference.h
 *
 * ABL → UE5 Blueprint translation for mc2_02.abl through mc2_12.abl.
 * This covers Act 1 of the MechCommander 2 campaign (Steiner missions).
 *
 * Read MC2MissionScript_Reference.h first — it documents the shared boilerplate
 * (common flags 0-24, music system, ABL function table, time unit conversion).
 * This file only documents mission-unique eternal flags and VO/video sequences.
 *
 * ===========================================================================
 * SHARED BOILERPLATE (identical in every mission, already in mc2_01 reference)
 * ===========================================================================
 *
 *  MissionFlags[0]  = ExitTimerSet
 *  MissionFlags[1]  = PlayerForceDead
 *  MissionFlags[2]  = ClanForceDead
 *  MissionFlags[3]  = AlliedForceDead
 *  MissionFlags[4]  = GeneralAlarm
 *  MissionFlags[5]  = Flag1 … MissionFlags[14] = Flag10
 *  MissionFlags[20] = AttackMusicTrigger1   (set by warrior scripts when group attacks)
 *  MissionFlags[21] = AttackMusicTrigger2
 *  MissionFlags[22] = AttackMusicTrigger3
 *
 *  Mission-unique eternal booleans start at MissionFlags[30].
 *
 * ===========================================================================
 * VO ASSET PATH CONVENTION
 * ===========================================================================
 *  "data\sound\mc2_NN_M.wav"  →  Content/Audio/VO/Tac/S_mc2_NN_M
 *  "tut_Xyz.wav"              →  Content/Audio/VO/Tac/S_tut_Xyz
 *  playVideo("FILE.bik")      →  Level Sequencer asset LevelSequence/Cutscenes/LS_FILE
 *
 * ===========================================================================
 * MISSION 02 — "Operation: Dagger Thrust" (Bandit Lake area)
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune6 → Content/Audio/Music/AmbientTrack6
 *  CombatTune pool: CombatTune4
 *
 *  Mission-unique MissionFlags (start at index 30):
 *    30  CallReinForce  — eternal; set TRUE when objective 6 complete (player
 *                         enters the base). Warrior scripts for Bandit Lake
 *                         reinforcements poll this flag to begin their attack.
 *
 *  VO Sequence (run on GameMode Tick, guarded by timer floats):
 *    startVO timer (5s after mission start):
 *      PlayVO("S_tut_3eii")    // "The RP Building is up ahead"
 *      → then set playVO_2 timer (startVO elapsed + 13s)
 *
 *    playVO_2 timer:
 *      PlayVideo("LS_V3G")     // Enemy mech cutscene near capture point
 *
 *    checkObjectiveStatus(7) == 1 (player enters turret area):
 *      PlayVO("S_tut_3hi")     // "Capture the Control Building"
 *
 *    checkObjectiveStatus(1) == 1 (South Defenders destroyed) + 3s delay:
 *      PlayVO("S_tut_3kii")    // "All bad dudes at south base destroyed"
 *      → set southDefendVO timer +10s
 *
 *    southDefendVO expires AND checkObjectiveStatus(2)==1 (Sensor captured):
 *      PlayVO("S_tut_3kij")    // "Capture the Sensor Tower"
 *
 *  Objective Index Map (from .fit file):
 *    0 = primary (win condition)
 *    1 = destroy south base defenders
 *    2 = capture sensor control
 *    3 = (secondary objective)
 *    6 = player enters base (area trigger)
 *    7 = player enters turret zone (area trigger)
 *
 *  UE5: BP_MC2Mission_02 extends AMC2GameMode.
 *    OnMissionStart → start 5s timer → PlayVO.
 *    Bind to OnObjectiveComplete(1) → start southDefendVO sequence.
 *    Bind to OnObjectiveComplete(6) → SetMissionFlag(30, true) [CallReinForce].
 *
 * ===========================================================================
 * MISSION 03 — uses mc2_12 script body
 * ===========================================================================
 *
 *  IMPORTANT: mc2_03.abl contains `module mc2_12` — the file was repurposed.
 *  Mission 3 in the campaign uses the same convoy-defense script as mission 12.
 *  Implement BP_MC2Mission_03 identical to BP_MC2Mission_12 (see below).
 *  The .fit objective and unit placement files differ; only the ABL logic is shared.
 *
 * ===========================================================================
 * MISSION 04 — "Operation: Final Solution" (Factory capture)
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune8 → Content/Audio/Music/AmbientTrack8
 *  CombatTune pool: CombatTune4 (90%) or AmbientTune for backup
 *
 *  Mission-unique MissionFlags (30+):
 *    30  Group0AttacksFlag   — warrior group 0 attacks when set
 *    31  Group1AttacksFlag
 *    32  Group2AttacksFlag
 *    33  Group3AttacksFlag
 *    34  Group4RunAwayFlag   — group 4 retreats when set
 *    35  GoToBaseFlag        — enemy units run to base when set
 *    36  NorthBridgeFlag     — player approached north bridge
 *    37  NorthBridge2Flag
 *    38  FoundPlayer         — enemy spotted player
 *    39  E1RunAway           — E1 group retreating
 *    40  Convoy1_Start       — convoy 1 begins moving
 *    41  Group4RunVO         — run VO played flag
 *
 *  Mission-unique eternal integers (area trigger IDs):
 *    FrontOfBaseTrigger   — isTriggerAreaHit() → UE5: ATriggerVolume overlap
 *    NorthBridgeTrigger   — north bridge area
 *    NorthBridge2Trigger
 *    CenterZoneTrigger
 *    SouthZoneTrigger
 *    NorthZoneTrigger
 *    ZoneEntered          — which zone the player entered last
 *
 *  In UE5: trigger areas → ATriggerVolume actors in the level, overlapped by
 *  AMC2Mover. Bind OnActorBeginOverlap on each volume, set the corresponding
 *  MissionFlag in the GameMode script.
 *
 *  VO / Video:
 *    FactoryCapturedVO timer → PlayVO("S_mc2_04_0")  // Liao Operator line 1
 *    Second VO → PlayVO("S_mc2_04_1")                // Liao Operator line 2
 *    Videos: "MC2_04A", "MC2_04B", "MC2_04C", "MC2_04D", "W11"
 *
 *  Objective gating:
 *    checkObjectiveStatus(4)==1 OR (5)==1 → trigger reinforcement group
 *    checkObjectiveStatus(2)==1 → music transition to SensorTune / combat
 *
 * ===========================================================================
 * MISSION 05 — "Operation: Northern Reach"
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune1
 *  CombatTune pool: (same generic system, no AttackMusicTrigger specifics)
 *
 *  Mission-unique MissionFlags (30+):
 *    30  NorthBridgeFlag
 *    31  MiddleOfBaseFlag
 *
 *  Trigger areas:
 *    MiddleOfBaseTrigger  — player enters base center
 *    NorthBridgeTrigger
 *
 *  VO / Video:
 *    checkObjectiveStatus(3)==1 → PlayVO("S_mc2_05_1")  // Tac officer VO
 *    Videos: "MC2_05A", "W02"
 *
 *  Objective gating:
 *    checkObjectiveStatus(3)==1 → start timer for VO
 *    checkObjectiveStatus(1)==1 → music → combat tracks
 *
 * ===========================================================================
 * MISSION 06 — "Operation: Hammer Strike" (Multi-group assault)
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune10
 *  CombatTune pool: CombatTune (generic)
 *
 *  Mission-unique MissionFlags (30+):
 *    30  goGroup1  — trigger group 1 to move/attack
 *    31  goGroup2
 *    32  goGroup3
 *    33  goGroup4
 *    34  goGroup5
 *    35  goGroup6
 *    36  goGroup7
 *
 *  Mission-unique eternal integers (guard unit references):
 *    guard1…guard7  — Part IDs of guard units; warrior scripts check these
 *    to know which unit to guard. In UE5: store Part indices or actor refs
 *    in a TArray<AMC2Mover*> GuardTargets on the GameMode.
 *
 *  Logic:
 *    checkObjectiveStatus(4)==1 → group2Timer = Now+delay → goGroup2=true → goGroup3=true
 *    checkObjectiveStatus(5)==1 → group3Timer → goGroup4/5/6/7=true (timed)
 *    checkObjectiveStatus(1)==1 → PlayVO("S_mc2_06")  // mission VO
 *
 *  UE5: goGroupN flags → SetMissionFlag(30+N-1, true). Warrior BT scripts for
 *  each group poll their flag via GameMode->ScriptGetMissionFlag().
 *
 * ===========================================================================
 * MISSION 07 — "Operation: Sand Trap" (Infantry ambush)
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune1
 *  No mission-specific VO files. No video cutscenes.
 *
 *  Mission-unique MissionFlags (30+):
 *    30  patrols_Area_Trigger   — player entered patrol zone
 *    31  infantry1_triggered
 *    32  infantry2_triggered
 *    33  infantry3_triggered
 *
 *  Logic:
 *    isTriggerAreaHit(PatrolsAreaTrigger) OR checkObjectiveStatus(0)==1
 *      → patrols_Area_Trigger = true → infantry groups activate
 *    infantry1/2/3_triggered are set by the warrior scripts themselves
 *    (the ambush timing logic is in the warrior .abl files, not the main script)
 *
 *  Objective gating for music:
 *    checkObjectiveStatus(0)==1 → combat music track 1
 *    checkObjectiveStatus(2)==1 → combat music track 2
 *
 * ===========================================================================
 * MISSION 08 — "Operation: Treachery" (Defend the base)
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune7
 *  No mission-specific VO files. No video cutscenes.
 *  No mission-unique eternal flags beyond the standard boilerplate.
 *
 *  The main script is minimal — only music selection logic.
 *  All behavior is driven by warrior sub-scripts for the attacking forces.
 *
 *  Objective gating for music:
 *    checkObjectiveStatus(2)==1 → PlayCountCheck → sensor music
 *    checkObjectiveStatus(0)==1 and (1)==1 → combat music
 *
 *  UE5: BP_MC2Mission_08 has no extra flags. OnMissionStart plays AmbientTrack7.
 *
 * ===========================================================================
 * MISSION 09 — "Operation: Rescue" (POW extraction)
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune9
 *  CombatTune pool: CombatTune (objectives 5 and 3 trigger different tracks)
 *
 *  Mission-unique MissionFlags (30+):
 *    30  ec1_Group_Attack    — escort group 1 attacks player when triggered
 *    31  ec3_Area_Trigger    — player enters EC3 area
 *    32  ec5_Event_Trigger   — event 5 triggered
 *    33  ec4_Detect_Player   — EC4 has detected the player
 *    34  defend_nosecone     — nosecone defense event triggered
 *
 *  Trigger areas:
 *    WeaponsFacilityTrigger   — integer area ID → ATriggerVolume
 *
 *  VO / Video:
 *    checkObjectiveStatus(0)==1 OR (1)==1 → then VO sequence:
 *      After delay: PlayVO("S_mc2_09_1")   // Boss asks for help
 *      → Video: "W05" (C-Bill reward cutscene)
 *      → Video: "MC2_09A"
 *      → Video: "MC2_09B"
 *    separate path: PlayVO("S_mc2_09_2")  // Boss says going to HQ
 *
 *  Objective gating:
 *    (0)==1 OR (1)==1 → ec1_Group_Attack sequence with VO
 *    (2)==1 → second VO
 *    (5)==1 → combat music track switch
 *
 * ===========================================================================
 * MISSION 10 — "Operation: Hammer and Anvil" (Gulag assault)
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune0
 *  CombatTune pool: tracks 1 and 3 (objectives 1 and 2 based)
 *
 *  Mission-unique MissionFlags (30+):
 *    30  ec1_Group_Run       — EC1 runs when triggered
 *    31  ec1_Group_Trigger   — EC1 has been triggered
 *    32  ec1_Group_Attack    — EC1 attacks
 *    33  ec2_attack_Gulag    — EC2 attacks the Gulag
 *    34  Convoy_Go_1         — convoy 1 begins
 *    35  Convoy_Go_2
 *    36  Reinforce_Go        — reinforcement group moves
 *    37  gotTOGulag          — player reached Gulag
 *    38  GoCam1              — camera sequence 1 trigger
 *    39  GoCam2
 *
 *  Trigger areas:
 *    frontOfGulagTriggerArea   — integer area ID
 *
 *  Camera system (Level Sequencer in UE5):
 *    eternal real[3] camRotation / camPosition — used for in-engine cinematic
 *    panning during mc2_10's multiple camera-fly sequences.
 *    GoCam1/GoCam2 triggers → Level Sequencer Play(LS_MC2_10_Cam1/2)
 *
 *  Videos: "MC2_10A", "MC2_10B", "MC2_10C", "MC2_10G", "MC2_10H"
 *
 *  Objective gating:
 *    (0)==1 → set ec1_Group_Trigger → delay → ec1_Group_Attack
 *    (1)==1 AND gotTOGulag → Convoy_Go_1=true
 *    (2)==1 → Reinforce_Go → combat music
 *
 * ===========================================================================
 * MISSION 11 — "Operation: Serpent's Den" (Sensor net + bridges)
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune2
 *  CombatTune pool: objective (4) triggers combat music
 *
 *  Mission-unique MissionFlags (30+):
 *    30  SensorVOTrigger    — sensor VO has been played
 *
 *  Trigger areas (integer IDs → ATriggerVolume):
 *    NorthBridgeTrigger
 *    CenterBridgeTrigger
 *    SouthBridgeTrigger
 *
 *  VO sequence:
 *    objectives (6)==0 OR (7)==0 AND SensorVOTrigger:
 *      PlayVO("S_mc2_11_1")    // "Sensor net is active"
 *    objectives (6)==1 OR (7)==1 (sensor destroyed) → Sensor_VO:
 *      PlayVO("S_mc2_11_5")    // "Sensor net destroyed"
 *    bridge area trigger AND objective (1)==0:
 *      PlayVO("S_mc2_11_3")    // Bridge warning
 *    Videos: "W03" (on objective 2), "W08" (on objective 3)
 *
 *  Objective index map:
 *    0  = primary (mission won)
 *    1  = bridge objective
 *    2  = secondary / bonus
 *    3  = secondary / bonus
 *    4  = (music gate)
 *    6  = north sensor (destroy to clear sensor net)
 *    7  = south sensor
 *
 * ===========================================================================
 * MISSION 12 — "Operation: Convoy Assault" (and mc2_03.abl reuse)
 * ===========================================================================
 *
 *  MissionStartTune: MissionStartTune5
 *  CombatTune pool: CombatTune5 (90%) or CombatTune0
 *
 *  Mission-unique MissionFlags (30+):
 *    30  CvA_Start_Trigger    — convoy A begins moving (10s after obj 1 complete)
 *    31  CvA_Start_Trigger2   — convoy A second wave (20s after obj 1 complete)
 *    32  CvB_Start_Trigger
 *    33  CvB_Start_Trigger2
 *    34  CvC_Start_Trigger
 *    35  CvC_Start_Trigger2
 *    36  CvD_Start_Trigger
 *    37  CvD_Start_Trigger2
 *    38  River_Area_Trigger   — player entered river zone
 *    39  City_Area_Trigger    — player entered city zone
 *    40  Island_Area_Trigger
 *    41  Hill_Area_Trigger
 *    42  BaseVO_Area_Trigger  — player entered base area
 *    43  MoverRunningToBase   — a convoy mover is fleeing to base
 *    44  CvA_End_Trigger      — convoy A reached endpoint (patrol done)
 *    45  CvA_End_Trigger2
 *    46  Reinf_G1_Trigger     — reinforcement group 1 deploys (obj 3 + 25s)
 *    47  Reinf_G1_Trigger2    — reinforcement group 1 second wave (obj 3 + 55s)
 *
 *  Trigger area integers: RiverAreaTrigger, CityAreaTrigger, IslandAreaTrigger,
 *  HillAreaTrigger, BaseVOAreaTrigger → ATriggerVolume actors in level.
 *
 *  VO sequence:
 *    BaseVO_Area_Trigger → PlayVO("S_mc2_12_3")   // Tac officer, base intro
 *    CvA completed run → PlayVO("S_mc2_12_0")     // Cash VO
 *    Second convoy event → PlayVO("S_mc2_12_1")
 *    Third event → PlayVO("S_mc2_12_2")
 *
 *  Convoy logic (all convoys follow same pattern):
 *    checkObjectiveStatus(1)==1 → CvATimerCheck → 10s → CvA_Start_Trigger=true
 *    checkObjectiveStatus(1)==1 → CvATimerCheck2 → 20s → CvA_Start_Trigger2=true
 *    checkObjectiveStatus(3)==1 → Reinf_TimerDelay → 25s → Reinf_G1_Trigger=true
 *    checkObjectiveStatus(3)==1 → Reinf_TimerDelay2 → 55s → Reinf_G1_Trigger2=true
 *
 *  In UE5: OnObjectiveComplete(1) → start FTimerHandle (10s) → SetMissionFlag(30,true)
 *          OnObjectiveComplete(1) → start FTimerHandle (20s) → SetMissionFlag(31,true)
 *          OnObjectiveComplete(3) → start FTimerHandle (25s) → SetMissionFlag(46,true)
 *          OnObjectiveComplete(3) → start FTimerHandle (55s) → SetMissionFlag(47,true)
 *
 *  NOTE: mc2_03.abl is module mc2_12 — mission 3 uses this same script.
 *  BP_MC2Mission_03 is a child of BP_MC2Mission_12 with different level actors.
 *
 * ===========================================================================
 * UE5 IMPLEMENTATION PATTERN (applies to all missions above)
 * ===========================================================================
 *
 *  1. Create BP_MC2Mission_NN : AMC2GameMode.
 *  2. In OnMissionStart:
 *       - StartTimer(0) for gametime tracking
 *       - Set up mission-specific FTimerHandle for timed VO (use GetWorldTimerManager())
 *       - PlayMusicTrack(MissionStartTuneN)
 *  3. Each objective that gates a flag: bind to GameMode's OnObjectiveComplete(N) event
 *     (fire when PrimaryObjectives[N]->Status == Complete).
 *  4. Mission flags read by warrior BT scripts:
 *       AMC2AIController → AMC2GameMode::ScriptGetMissionFlag(Index)
 *  5. Trigger areas → ATriggerVolume placed in level; OnActorBeginOverlap fires
 *     GameMode->ScriptSetMissionFlag(Index, true).
 *  6. Videos → PlayLevelSequence(LevelSequence'/Game/Cutscenes/LS_Xxx.LS_Xxx')
 *     via ULevelSequencePlayer in the GameMode.
 *
 * ===========================================================================
 * WARRIOR SCRIPT POLLING
 * ===========================================================================
 *
 *  Warrior ABL scripts (mc2_NN_GroupName.abl) run per-unit in the Warriors folder.
 *  They poll eternal boolean flags via ABL module-shared memory.
 *  In UE5 this is replaced by AMC2AIController polling GameMode mission flags:
 *
 *    // In BTTask or AIController Tick:
 *    AMC2GameMode* GM = Cast<AMC2GameMode>(GetWorld()->GetAuthGameMode());
 *    bool bGoGroup = GM && GM->ScriptGetMissionFlag(30); // goGroup1
 *    if (bGoGroup) { /* issue attack order */ }
 *
 *  Each group of warrior units in a mission level is a set of AMC2Mover actors
 *  controlled by separate AMC2AIController instances, all reading the same flags.
 *
 * ===========================================================================
 * MUSIC TUNE INDEX TABLE (from sndconst.abi)
 * ===========================================================================
 *
 *  ABL constant        UE5 asset path
 *  MissionStartTune0   Content/Audio/Music/S_MissionStart_00
 *  MissionStartTune1   Content/Audio/Music/S_MissionStart_01
 *  MissionStartTune2   Content/Audio/Music/S_MissionStart_02
 *  MissionStartTune5   Content/Audio/Music/S_MissionStart_05
 *  MissionStartTune6   Content/Audio/Music/S_MissionStart_06
 *  MissionStartTune7   Content/Audio/Music/S_MissionStart_07
 *  MissionStartTune8   Content/Audio/Music/S_MissionStart_08
 *  MissionStartTune9   Content/Audio/Music/S_MissionStart_09
 *  MissionStartTune10  Content/Audio/Music/S_MissionStart_10
 *  CombatTune0         Content/Audio/Music/S_Combat_00
 *  CombatTune4         Content/Audio/Music/S_Combat_04
 *  CombatTune5         Content/Audio/Music/S_Combat_05
 *  AmbientTune3        Content/Audio/Music/S_Ambient_03
 *  SensorTune0         Content/Audio/Music/S_Sensor_00
 *  SensorTune3         Content/Audio/Music/S_Sensor_03
 *  MissionWonTune0     Content/Audio/Music/S_MissionWon
 *  MissionLostTune0    Content/Audio/Music/S_MissionLost
 */

// This file is documentation only — not compiled.
