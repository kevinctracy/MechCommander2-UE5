/**
 * MC2TutorialScript_Reference.h
 *
 * ABL → UE5 Blueprint translation for tutorial0.abl, tutorial1.abl, tutorial2.abl.
 * These are LOGISTICS (between-mission) tutorial scripts, not in-mission scripts.
 * They fire VO cues when the player first navigates to each logistics screen.
 *
 * ===========================================================================
 * KEY DIFFERENCE: Logistics vs Mission Scripts
 * ===========================================================================
 *
 *  mc2_01.abl and other mission scripts run inside a mission level (in-game).
 *  tutorial*.abl scripts run in the LOGISTICS SHELL (menus between missions).
 *  The ABL variable `logisticsScreenId` has no direct UE5 equivalent;
 *  it must be replaced with delegates/events fired by each UMG widget.
 *
 * ===========================================================================
 * SCREEN ID → UMG WIDGET MAPPING
 * ===========================================================================
 *
 *  ABL constant                   Value   UMG Widget
 *  pMissionSelectionScreen        1       WBP_MissionBriefing / WBP_MissionSelect
 *  pBriefingScreen                11      WBP_MissionBriefing
 *  pMechBayScreen                 21      WBP_MechBay
 *  pPilotSelectionScreen          31      WBP_PilotReady
 *  pPurchaseMechScreen            20      WBP_PurchaseMech (or part of WBP_MechBay)
 *  pMechLabScreen                 22      WBP_MechLab
 *  pLoadScreen                    41      WBP_Loading
 *
 * ===========================================================================
 * tutorial0.abl — Minimal shell
 * ===========================================================================
 *
 *  Only one eternal boolean: firstTimeMechBay = true (init).
 *  Main code: return(0) — does nothing.
 *
 *  UE5: No Blueprint needed for tutorial0. The firstTimeMechBay flag is
 *  already handled by tutorial1. Skip this file entirely.
 *
 * ===========================================================================
 * tutorial1.abl — Logistics Tutorial VO (MechBay, MechLab, PilotSelect, etc.)
 * ===========================================================================
 *
 *  Variables → Campaign flags (UMC2LogisticsSubsystem::SetCampaignFlag):
 *
 *    Flag  ABL name                 Trigger
 *    ---   --------                 -------
 *    30    firstTimeMechBay         true until WBP_MechBay opened for first time
 *    31    firstTimeMissionSelect   true until WBP_MissionSelect opened for first time
 *    32    firstTimeBriefing        true until WBP_MissionBriefing opened
 *    33    firstTimePilotSelect     true until WBP_PilotReady opened
 *    34    firstTimePurchase        true until purchase screen opened
 *    35    firstTimeMechLab         true until WBP_MechLab opened
 *    36    firstTimeLoadScreen      true until loading screen shown
 *
 *  Stage integers (per-screen state machines):
 *    mechBayStage, pilotSelectStage, briefingStage, purchaseStage, mechLabStage
 *    → local Blueprint int vars on each widget's "Tutorial" component
 *
 *  MECHBAY TUTORIAL (WBP_MechBay — on NativeConstruct or first-open event):
 *
 *    On first open (firstTimeMechBay = true):
 *      StopVO()
 *      PlayVO("tut_2b")          // "Welcome to the MechBay"
 *      firstTimeMechBay = false
 *
 *    mechBayStage state machine (runs in Tick or via VO end delegate):
 *      stage 0: wait for VO to finish → stage 1
 *                  (logisticsAnimationCallout(5,...) = highlight repair button → skip in UE5)
 *      stage 1: wait for callout dismiss → PlayVO("tut_2c") → stage 2
 *      stage 2: wait for VO finish → stage 3
 *                  (callout 23 = highlight pilot slot)
 *      stage 3: wait for callout → stage 4
 *                  (callout 17 = highlight mech status)
 *      stage 4: wait → startTime = Now → stage 5
 *      stage 5: wait 5s → stage 6
 *      stage 6: (tut_2d commented out) → stage 7
 *      stage 7: (callout 51 = highlight launch button) → stage 8
 *      stage 8: (tut_2e commented out) → stage 9
 *      stage 9: (callout 18) → done
 *
 *    REWIND RULES (when player leaves MechBay before finishing):
 *      If stage == 0 and not first time → reset firstTimeMechBay = true
 *      If stage == 2 → rewind to stage 1
 *      If stage == 7 → rewind to stage 6
 *      If stage == 9 → rewind to stage 8
 *
 *  PILOT SELECT TUTORIAL (WBP_PilotReady):
 *
 *    On first open:
 *      StopVO()
 *      firstTimePilotSelect = false
 *
 *    pilotSelectStage state machine:
 *      stage 0: startTime = Now → stage 1
 *      stage 1: wait 3s → stage 2
 *      stage 2: PlayVO("tut_2f") → stage 3    // "Assign pilots to mechs"
 *      stage 3: wait for VO → (callout 0 = highlight pilot portrait) → stage 4
 *      stage 4: wait for callout → startTime = Now → stage 5
 *      stage 5: wait 2s → stage 6
 *      stage 6: (callout 15 = highlight assignment button) → stage 7
 *
 *    REWIND RULES:
 *      If stage == 3 → rewind to 2
 *      If stage == 7 → rewind to 6
 *
 *  BRIEFING TUTORIAL (WBP_MissionBriefing):
 *
 *    On first open (firstTimeBriefing):
 *      PlayVO("tut_2a")         // "Here is your mission briefing"
 *      firstTimeBriefing = false
 *
 *  PURCHASE/MECHLAB TUTORIALS (WBP_PurchaseMech / WBP_MechLab):
 *
 *    On first open (firstTimePurchase):
 *      StopVO(); firstTimePurchase = false
 *      purchaseStage 0: startTime = Now → 1
 *      purchaseStage 1: wait 3s → 2
 *      purchaseStage 2: PlayVO("tut_5a") → done
 *
 *    On first open (firstTimeMechLab):
 *      StopVO(); firstTimeMechLab = false
 *      mechLabStage 0: startTime = Now → 1
 *      mechLabStage 1: wait 3s → 2
 *      mechLabStage 2: PlayVO("tut_5b") → 3
 *      mechLabStage 3: wait for VO → 4  (callout 22 commented out)
 *      mechLabStage 4: wait for callout → PlayVO("tut_5c") → 5
 *      mechLabStage 5: wait for VO → 6  (callout 21 commented out)
 *      mechLabStage 6: wait for callout → PlayVO("tut_5d") → done
 *
 * ===========================================================================
 * UE5 IMPLEMENTATION APPROACH
 * ===========================================================================
 *
 *  Do NOT create a single global Tutorial Script actor.
 *  Instead: each UMG widget owns a "UTutorialHelper" actor component that
 *  runs its screen-specific state machine on NativeConstruct.
 *
 *  class UMC2TutorialHelper : public UActorComponent
 *  {
 *    // Inject via widget's owning controller
 *    void OnScreenOpened(int32 ScreenID);    // call from widget NativeConstruct
 *    void OnVOFinished();                    // bind to UAudioComponent::OnAudioFinished
 *    void Tick(float DeltaTime);             // for timer-based waits
 *
 *    // Reads/writes campaign flags via UMC2LogisticsSubsystem
 *    bool IsFirstTime(int32 FlagIndex);
 *    void SetFirstTimeDone(int32 FlagIndex);
 *  }
 *
 *  logisticsAnimationCallout(N, ...) → UI highlight animation on widget element N.
 *  In UE5: call BlueprintImplementableEvent OnShowCallout(int32 ElementID) on the widget.
 *  Callout IDs: 5=repair, 17=mech status, 18=nav, 22=slot grid, 23=pilot slot, 51=launch btn, 0=pilot portrait, 15=assign btn.
 *
 * ===========================================================================
 * tutorial2.abl — (check for additional content)
 * ===========================================================================
 *
 *  tutorial2.abl is the in-mission tutorial (similar to mc2_01.abl).
 *  It runs inside a combat mission level.
 *  Use the same mc2_01.abl Blueprint approach documented in MC2MissionScript_Reference.h.
 *  Key difference: tutorial2 focuses on combat tutorial objectives (move, attack, etc.)
 *  rather than logistics screens. Implement as BP_Tutorial2 child of AMC2GameMode.
 *
 * ===========================================================================
 * VO FILE MAPPING
 * ===========================================================================
 *
 *  ABL wave name   UE5 asset path
 *  "tut_2a"        Content/Audio/VO/Tac/S_tut_2a
 *  "tut_2b"        Content/Audio/VO/Tac/S_tut_2b
 *  "tut_2c"        Content/Audio/VO/Tac/S_tut_2c
 *  "tut_2f"        Content/Audio/VO/Tac/S_tut_2f
 *  "tut_5a"        Content/Audio/VO/Tac/S_tut_5a
 *  "tut_5b"        Content/Audio/VO/Tac/S_tut_5b
 *  "tut_5c"        Content/Audio/VO/Tac/S_tut_5c
 *  "tut_5d"        Content/Audio/VO/Tac/S_tut_5d
 */

// This file is documentation only — not compiled.
