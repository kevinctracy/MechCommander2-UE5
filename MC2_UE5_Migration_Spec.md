# MechCommander 2 → Unreal Engine 5 Migration Spec

**Date:** 2026-04-25  
**Engine Target:** UE5.4+ with Blueprints  
**Source:** DirectX 8 C++ codebase, ~400 source files across MCLib + Code  
**Game Type:** Real-time tactical mech combat, isometric/top-down camera, single-player campaign + multiplayer

---

## Asset Inventory

| Asset Type | Count | Format | Import Path |
|---|---|---|---|
| Source textures | 2,315 | `.tga` | Direct UE import |
| Runtime textures | 100 | `.txm` (LZW-compressed BGRA) | ✅ Python extractor written (`txm_extract.py`) |
| Audio files | 1,770 | `.wav` | Direct UE import |
| UI layouts | 135 | `.fit` (text INI) | Manual UMG rebuild |
| Object definitions | ~80 | `.fit` (text INI) | Convert to UE Data Tables |
| Mission scripts | 35 | `.abl` (custom language) | Rebuild as Blueprint Event Graphs |
| 3D models | 2,947 ASE files | `.ase` (3DS Max ASCII) | ✅ 1,037 groups found; `ase_to_fbx.py` written |
| Effect definitions | 1 | `.fx` (GosFX text) | Rebuild as Niagara systems |
| Terrain heightmaps | 4 | `.tga` (60x60–120x120) | UE Landscape import |

---

## Architecture Mapping: MC2 → UE5

### Game Objects

| MC2 Class | UE5 Class | Notes |
|---|---|---|
| `GameObject` | `AActor` | Base actor |
| `Mover` | `AMC2Mover : APawn` | Movement + sensors + weapons base |
| `BattleMech` | `ABP_BattleMech` (Blueprint) | Extends `AMC2Mover` |
| `GroundVehicle` | `ABP_GroundVehicle` (Blueprint) | Extends `AMC2Mover` |
| `Turret` | `ABP_Turret` (Blueprint) | Static mover variant |
| `Building` | `ABP_Building` (Blueprint) | Destructible building actor |
| `WeaponBolt` | `AMC2Projectile : AActor` | Hitscan + ballistic projectile |
| `Carnage` (fire/explosion) | `AMC2Carnage : AActor` | Wraps Niagara system |

### AI & Pathfinding

| MC2 System | UE5 Equivalent | Notes |
|---|---|---|
| `MovePathManager` queue | `UAIMoveTo` + `UNavigationSystemV1` | NavMesh replaces custom grid |
| `MechWarrior` pilot AI | `AMC2AIController : AAIController` | + Behavior Tree asset |
| `ContactInfo` per-unit | `UContactComponent` (custom) | Actor component |
| `SensorSystem` + ECM | `USensorComponent` (custom) | Checks line-of-sight via sphere sweeps |
| `Team` | `ATeamInfo` in `AGameState` | Team allegiances |

### Mission & Campaign

| MC2 System | UE5 Equivalent | Notes |
|---|---|---|
| `Mission` | `AMC2GameMode` | Manages lifecycle + win/loss |
| `Objective` (23 condition types) | `UMC2ObjectiveComponent` | On `AMC2GameMode` |
| ABL mission scripts | Blueprint Event Graph per mission | ~480 lines ABL avg = ~60-80 Blueprint nodes |
| `Part` (unit spawn record) | `FMC2PartData` struct in Data Asset | |
| Campaign state | `UMC2SaveGame : USaveGame` | |
| `LogisticsData` | `UMC2LogisticsSubsystem` | Game Instance Subsystem |
| `LogisticsMech` | `FMC2MechRecord` struct | In Data Asset |
| `LogisticsPilot` | `FMC2PilotRecord` struct | In Data Asset |
| `LogisticsVariant` | `UMC2VariantDataAsset` | Weapon loadout per mech chassis |
| `LogisticsComponent` | `UMC2ComponentDataAsset` | Weapons, engines, heatsinks |

### Rendering & Terrain

| MC2 System | UE5 Equivalent | Notes |
|---|---|---|
| TGL / DirectX 8 renderer | UE5 renderer (built-in) | No work needed |
| `Terrain` quad-tree | `ALandscape` | Import heightmaps, paint material layers |
| `TerrainTextures` | Landscape Material with layers | Blend dirt/grass/rock/water |
| `CraterManager` deformation | Runtime Virtual Texture + decal craters | True deformation expensive in UE; decal craters look same |
| `Mech3D` + 25 gesture animations | Animation Blueprint + Blend Space | Walk/run/limp/fall/jump states |
| GosFX particle effects | Niagara Particle System | Weapons fire, explosions, dust |
| TXM texture cache | UE texture streaming (built-in) | After extraction |

### UI

| MC2 Screen | UE5 UMG Widget | FIT source file |
|---|---|---|
| Main menu | `WBP_MainMenu` | `mainscreen.fit` |
| Mission HUD | `WBP_HUD` | `buttonLayout640.fit` |
| Tactical map | `WBP_TacMap` | Minimap component |
| MechBay | `WBP_MechBay` | `MCL_GN_*.fit` files |
| MechLab (loadout) | `WBP_MechLab` | |
| Pilot screens | `WBP_PilotReady`, `WBP_PilotReview` | `MCL_PR_*.fit` files |
| Salvage | `WBP_Salvage` | |
| Mission briefing | `WBP_MissionBriefing` | |
| Multiplayer lobby | `WBP_MPLobby` | `MCL_MP_*.fit` files |
| Loading screen | `WBP_Loading` | `mcl_loadingscreen.fit` |

---

## Phased Delivery Plan

### Phase 0 — Asset Extraction Pipeline
**Goal:** Get all assets into UE-importable formats before any gameplay work.

**Duration estimate:** 2–4 weeks (tooling-heavy)

#### P0.1 — TXM Texture Extractor
The `.txm` files in `FinalBuild/data/textures/` are raw ARGB pixel data (passed to `gos_NewTextureFromMemory`) LZ-compressed in a custom format derived from `txmmgr.cpp`. The on-disk file is the raw uncompressed ARGB buffer stored with the `width` field containing `0xf0000000 | fileSize`.

- [x] **P0.1.1** Write Python script to read `.txm` files — 9-12 bit variable-width LZW decompressor + BGRA→RGBA PNG converter (`txm_extract.py`).
- [x] **P0.1.2** Run extractor against all 100 `.txm` files — 100/100 extracted successfully to 64×64 PNGs.
- [ ] **P0.1.3** Import extracted PNGs into UE5 texture assets, set compression type (BC3 for alpha, BC1 for opaque).

**Note:** The 2,315 source `.tga` files can be imported directly — do that first for quick wins.

#### P0.2 — FIT Object Definition Converter
The `.fit` files use a text INI format (`FITini` header, typed key-value pairs with `l/f/st/uc/b/c` prefixes). Object definitions (mechs, vehicles, buildings) contain all gameplay stats.

- [x] **P0.2.1** Write Python parser for the FIT INI format (`fit_convert.py`) — handles typed key-value pairs and multi-column CSV layout.
- [x] **P0.2.2** Generate UE5 Data Table CSV files: `DT_Components`(80), `DT_MechChassis`(34), `DT_MechVariants`(42), `DT_VehicleTypes`(67), `DT_BuildingTypes`(1088), `DT_Pilots`(51).
- [x] **P0.2.3** Define UE row structs: `FMC2ComponentRow`, `FMC2MechChassisRow`, `FMC2MechVariantRow`, `FMC2VehicleTypeRow`, `FMC2BuildingTypeRow`, `FMC2PilotRow` (`MC2DataTableRows.h`).
- [ ] **P0.2.4** Import CSV Data Tables into UE.

#### P0.3 — 3D Model Extraction
The `object2.pak` and `feet.pak` files contain compiled `TG_TypeMultiShape` binary geometry. This is the highest-risk item — the format is proprietary.

- [x] **P0.3.1** ✅ Risk eliminated — 2,947 `.ase` source files found in `Source/Data/TGL/`.
- [x] **P0.3.2** ✅ ASE files confirmed; `ase_to_fbx.py` written (Blender batch converter). Groups: base(571), animation(644), lod(568), destroyed(1100), arm(64).
- [x] **P0.3.3** N/A — PAK extractor not needed.
- [ ] **P0.3.4** Run `ase_to_fbx.py` via Blender to produce FBX files; import into UE Skeletal Mesh assets.
- [ ] **P0.3.5** Import animations as UE Animation Sequences, grouped by the 25 gesture types from `mech3d.h`.

#### P0.4 — Audio Import
- [x] **P0.4.1** `audio_organize.py` written — categorizes 1,770 WAVs into 12 categories (Weapons, Ambient, Music, VO/Pilots, VO/Tac, UI, Mech, Vehicle, etc.). 1,100 pilot VO lines correctly identified. Ready for batch import.
- [ ] **P0.4.2** Create Sound Cue assets for multi-variant sounds (e.g. weapon fire variants).
- [ ] **P0.4.3** Create Sound Classes and Sound Mix for master volume controls.

---

### Phase 1 — UE5 Project Skeleton
**Goal:** Walking mech on a landscape with camera. Nothing functional, just infrastructure.

**Duration estimate:** 1–2 weeks

- [x] **P1.1** UE5.4 project created: `MechCommander2.uproject` with EnhancedInput, Niagara, OnlineSubsystem plugins.
- [x] **P1.2** Source folder structure: `Public/Units/`, `Public/Combat/`, `Public/AI/`, `Public/Camera/`, `Public/Mission/`, `Public/UI/`, `Public/Campaign/`, `Public/Online/`.
- [x] **P1.3** `AMC2GameMode`, `AMC2PlayerController`, `AMC2GameState` C++ base classes written.
- [x] **P1.4** `AMC2CameraActor` with edge scroll (20px margin), WASD pan, scroll zoom (400-2500 UU, step 200), middle-mouse rotation (pitch clamped -75° to -20°).
- [ ] **P1.5** Import terrain heightmap as UE Landscape.
- [ ] **P1.6** Place mech skeletal mesh on landscape.
- [ ] **P1.7** Set up Git LFS for large binary assets.

---

### Phase 2 — Core Unit System
**Goal:** Selectable mechs that move to clicked locations via NavMesh.

**Duration estimate:** 3–4 weeks

#### P2.1 — Base Mover C++ Class
Derived from `mover.h/cpp`. Key data to port:
- Armor zones: `NUM_MECH_HIT_ARCS` (4 arcs: front/rear/left/right) × `NUM_MECH_HIT_SECTIONS` (3 sections: top/mid/bottom)
- Component slots (from `cmponent.h` — COMPONENT_FORM_WEAPON, ENGINE, HEATSINK, etc.)
- Speed parameters: `MaxVelocity`, `MaxAccel` from FIT data
- Heat system: heat generation/dissipation per weapon type + heatsinks

Tasks:
- [x] **P2.1.1** `AMC2Mover : APawn` with 12-zone mech armor (`FMC2ArmorZone` × 4 arcs × 3 sections), heat system, order system, `bIsDestroyed`.
- [x] **P2.1.2** `UMC2HealthComponent` — bone→zone heuristic lookup, armor absorb → IS overflow → CriticalHitChance(0.25) → section destroyed event.
- [x] **P2.1.3** `UFloatingPawnMovement` component; shutdown zeroes `MaxSpeed`; leg damage applies `LimpSpeedMultiplier`.
- [x] **P2.1.4** `USensorComponent` — sphere overlap every 0.5s, LOS traces, ECM jamming, FoW tile updates.
- [x] **P2.1.5** `UWeaponHardpointComponent` — energy/ballistic/missile types, cooldown, ammo, heat generation, `GetHeatSinkBonus()`.

#### P2.2 — BattleMech Blueprint
- [ ] **P2.2.1** Create `BP_BattleMech` extending `AMC2Mover`. Assign Skeletal Mesh, Physics Asset, Animation Blueprint.
- [ ] **P2.2.2** Create `ABP_BattleMech` Animation Blueprint with states: Idle, Walk, Run, Limp, TorsoTwist, Fall, GetUp, Jump. Use Blend Spaces for speed-driven walk/run blend.
- [ ] **P2.2.3** Wire torso yaw rotation (aim direction) independently of leg direction, matching MC2's body section twist.
- [x] **P2.2.4** Paint system: `PrimaryColor` / `SecondaryColor` (FLinearColor) on `AMC2Mover`. `ApplyPaintColors()` creates `UMaterialInstanceDynamic` per mesh slot, sets "PrimaryColor"/"SecondaryColor" vector params. Called on `BeginPlay` and any time colors change. Blueprint sets colors from DT_Pilots or team data.

#### P2.3 — GroundVehicle Blueprint
- [x] **P2.3.1** `AMC2GroundVehicle : AMC2Mover` — turret yaw rotation via `SetBoneRotationByName("turret")` each tick. `AimTurretAt(WorldTarget)` clamps to `MaxTurretYaw` ±half. `TurretYawRate` interpolation. BP_GroundVehicle extends this.
- [x] **P2.3.2** 5-arc vehicle armor: `FMC2VehicleArmor` (Arc enum + armor + IS per arc). `ApplyVehicleDamage(Damage, Arc)` — armor absorbs first, IS remainder. Any arc IS=0 → `OnDestroyed_MC2()`. `GetHitArcFromDirection(origin)` derives Front/Rear/Left/Right from dot products.

#### P2.4 — Selection & Orders
- [x] **P2.4.1** Box-select in `AMC2PlayerController`: drag < 5px = single click, else rectangle select via screen projection.
- [x] **P2.4.2** Right-click move order via `GetHitResultUnderCursorByChannel` → `ReceiveMoveOrder`.
- [x] **P2.4.3** Formation movement: rows of 4, 350 UU spacing, Right vector perpendicular to move direction.
- [x] **P2.4.4** Attack-move order: `ReceiveAttackMoveOrder(Destination)` sets `OrderType=AttackMove`, moves to location; BT's `BTTask_FindNearestEnemy` interrupts movement when sensor contact found mid-route.
- [x] **P2.4.5** Guard order: `ReceiveGuardOrder(Position)` — moves to position, AI returns to `BB_HOME_POSITION` if enemy retreats out of range (handled in BT guard branch).

---

### Phase 3 — Combat System
**Goal:** Mechs shoot at each other; correct damage model; units die.

**Duration estimate:** 3–4 weeks

#### P3.1 — Weapon System
From `weaponbolt.h` and component types in `cmponent.h` (WEAPON_ENERGY, WEAPON_BALLISTIC, WEAPON_MISSILE, WEAPON_ANTIAIRCRAFT):

- [x] **P3.1.1** `AMC2Projectile` — `USphereComponent` + `UProjectileMovementComponent`, gravity=0 default, splash damage via `OverlapMultiByObjectType`.
- [x] **P3.1.2** Energy weapons: `LineTraceSingleByChannel` from weapon hardpoint to target (in `UWeaponHardpointComponent`).
- [x] **P3.1.3** `AMC2Missile` — `bIsHomingProjectile=true`, `ArmTime=0.3s`, `Intercept()` for AMS.
- [x] **P3.1.4** Heat generation per shot via `AddHeat()`; shutdown at 95% threshold zeroes `MaxSpeed`.
- [x] **P3.1.5** Ammo depletion tracked per hardpoint; `CanFire()` checks ammo, cooldown, shutdown.
- [x] **P3.1.6** Weapon cooldown timer per `UWeaponHardpointComponent` via `CooldownRemaining` float.

#### P3.2 — Damage Model
From mech armor zone constants and vehicle armor sections:
- [x] **P3.2.1** `UMC2HealthComponent::ApplyDamage` — bone → zone lookup (`FindZoneForBone` with heuristic fallback) → armor absorb → IS overflow → `OnSectionDestroyed` event.
- [x] **P3.2.2** `TriggerCriticalHit` + `ApplyCritConsequences` — rolls `CriticalHitChance(0.25)` per section; identifies engine/gyro/weapon/actuator/cockpit crits from bone name.
- [x] **P3.2.3** Engine crit: -25% speed (1st), immobilize + shutdown (2nd). Gyro crit: `RollPilotingCheck(CritCount)`. Leg crit: `MaxMoveSpeed *= LimpSpeedMultiplier`.
- [x] **P3.2.4** Cockpit/head crit → `Pilot->TakeInjury(1)`; gyro crit → piloting check failure applies injury via `UMC2PilotComponent`.
- [x] **P3.2.5** `OnDestroyed_MC2`: spawns `ExplosionEffect` Niagara via `SpawnSystemAtLocation`, hides SkeletalMesh, attaches `DestroyedMesh` static component. `ExplosionScale` matches small/medium/large carnage sizes. `BP_OnDestroyed` available for per-chassis Blueprint override.

#### P3.3 — Effects
- [x] **P3.3.1** `UMC2FXLibrary` + `EMC2FXType` enum — 12 FX types (Explosion S/M/L, Muzzle Energy/Ballistic/Missile, SmokeTrail, Fire, DustCloud, HitSpark, WaterSplash, JumpJet). `SpawnFX` / `SpawnFXAttached` load NS_MC2_* assets by name convention from /Game/FX/. `GetExplosionScaleForBR` maps BR 1-20 → scale 0.5-2.0. Niagara assets themselves created in editor.
- [x] **P3.3.2** `AMC2Carnage` — 7 carnage types, `OnNiagaraFinished` auto-destroy, `SpawnCarnage()` static factory.
- [x] **P3.3.3** Hit sparks/impact decals: `AMC2Projectile::OnHit` uses `UMC2FXLibrary::HitSpark` always; non-mover hits also spawn `CraterDecalMaterial` via `SpawnDecalAtLocation` (30s lifetime, 80cm default size). `CraterDecalMaterial/Size/Lifetime` properties on projectile; assign M_BulletCrater in BP subclasses.
- [x] **P3.3.4** `UMC2AnimNotify_FootstepDust` — placed at foot-plant frame on walk/run anim sequences. `FootSocket` (foot_l/foot_r) + `DustScale`. Spawns `NS_MC2_Dust_Cloud` 20cm below socket world position.

#### P3.4 — Buildings & Turrets
- [x] **P3.4.1** `AMC2Building` — `UStaticMeshComponent` + `UMC2HealthComponent`, `DestroyBuilding()` swaps destroyed mesh, disables collision, spawns carnage, `ApplyRadialDamage`. `FOnBuildingDestroyed` delegate.
- [x] **P3.4.2** `AMC2Turret : AMC2Mover` — base mesh (static) + rotating turret mesh, `AimTurretAtTarget()` via `FMath::FixedTurn`, `MaxTurretYaw` clamp, `FireIfReady()`, overrides move/guard as no-ops.
- [ ] **P3.4.3** Implement turret auto-attack AI: simple BTTask_FindNearestEnemy → BTTask_AttackTarget Behavior Tree.

---

### Phase 4 — AI & Pathfinding
**Goal:** Enemy units navigate, engage player, and retreat correctly.

**Duration estimate:** 3–4 weeks

#### P4.1 — Navigation
- [x] **P4.1.1** Configure UE `RecastNavMesh` to match MC2 terrain: set agent radius/height for mech sizes (light/medium/heavy/assault). `DefaultEngine.ini` — BattleMech agent (radius=180, height=900), TileSize=1200, WalkableSlope=45°. Weight-class radii: Light=140, Medium=180, Heavy=220, Assault=260.
- [x] **P4.1.2** `UMC2NavQueryFilter_Light/Medium/Heavy` — 3 filter subclasses, `SetMaxSearchNodes(2048)`, per weight-class area costs so heavy mechs avoid narrow corridors.
- [x] **P4.1.3** Implement formation pathfinding: lead unit queries NavMesh, followers offset from lead path. `AMC2AIController::SetFormationSlot(Leader, LocalOffset)` — computes world goal = leader's move destination + LocalOffset rotated by leader yaw. `ClearFormation()` / `IsInFormation()`. Goal injected into BB_MOVE_TARGET each blackboard tick.
- [ ] **P4.1.4** Implement terrain avoidance for water/cliffs using NavModifierVolumes placed on landscape.

#### P4.2 — Enemy AI Behavior Tree
Replicates `MechWarrior` pilot logic from `warrior.h/cpp`:
- [x] **P4.2.1** `AMC2AIController` — `EnemyBehaviorTree` / `AllyBehaviorTree`, blackboard key macros (BB_TARGET_ACTOR, BB_MOVE_TARGET, BB_ORDER_TYPE, BB_CURRENT_HEAT, BB_NEAREST_ENEMY, BB_HAS_LOS, BB_IS_SHUTDOWN, BB_HOME_POSITION), timer-based blackboard update every 0.25s. BT asset to be created in UE editor.
- [x] **P4.2.2** `BTTask_FindNearestEnemy` — queries `USensorComponent::GetContacts()`, filters by `bRequireLOS`, sets `BB_NEAREST_ENEMY/BB_TARGET_ACTOR/BB_HAS_LOS`.
- [x] **P4.2.3** `BTTask_MoveToAttackRange` — reads `WeaponHardpoints[i]->WeaponRange`, `StopDistance = MaxRange - RangeBuffer`, latent move via `FAIMoveRequest`, ticks until in range.
- [x] **P4.2.4** `BTTask_FireAtTarget` — suppresses at `HeatPct >= 0.85`, iterates hardpoints (bDestroyed/cooldown/ammo/dist checks), calls `WC->FireAt(Target)`.
- [x] **P4.2.5** `BTTask_Patrol` + `AMC2WaypointActor` — nearest waypoint selection, `NextWaypoint` chain, `WaitDuration`, `GetChainStart()` loop, editor billboard component.
- [x] **P4.2.6** Implement pilot skill modifiers: `BTTask_FireAtTarget` reads `UMC2PilotComponent::GetBaseHitChance()` (Gunnery 4 = 58%). Range band penalty: -8% per half-range band past optimal. Per-weapon FRand roll — miss skips `FireAt` but cooldown still consumed (visual fire plays).

#### P4.3 — Sensor & Contact System
From `contact.h` and sensor system in MC2:
- [x] **P4.3.1** `USensorComponent` — sphere overlap every `SensorTickInterval`(0.5s), LOS line traces, `FMC2ContactInfo { Unit, DistanceSq, bHasLOS, LastSeenTime }`.
- [x] **P4.3.2** ECM — checks target `SensorComponent->bHasECM`, applies `ECMRangeReduction` within `ECMRadius`.
- [x] **P4.3.3** FoW — 256×256×8-team bit-packed in `AMC2GameState::FogOfWarBits`; sensor component sets tiles visible in radius circle.

---

### Phase 5 — Mission System
**Goal:** A complete playable mission with win/lose conditions.

**Duration estimate:** 4–5 weeks

#### P5.1 — Mission Framework
From `mission.h` and `Objective.h`:
- [x] **P5.1.1** `AMC2GameMode` — `Parts[]`, `PrimaryObjectives[]`, `SecondaryObjectives[]`, `BonusObjectives[]` (Instanced), ABL script API (ScriptSetMissionFlag, ScriptGetTimer, ScriptSpawnUnit, ScriptSetMissionResult).
- [x] **P5.1.2** `SpawnAllParts()` — iterates Parts, spawns AMC2Mover subclasses, updates `AMC2GameState` team tallies.
- [x] **P5.1.3** 6 result states in `EMC2MissionResult` enum; `ApplyMissionResult()` sets GameState and fires `OnMissionResult` event.
- [x] **P5.1.4** `AMC2MissionVolume : ATriggerVolume` — `AreaID` (FName), `FilterTeamIndex`, `RequiredUnitCount`. `OnMoverEntered/Exited/OnTeamFullyInside` delegates. `GetUnitCountInside(TeamIndex)` / `IsUnitInside()`. `CheckTeamThreshold` fires team delegate when count ≥ required. Replaces ABL `isTriggerAreaHit(N)`.

#### P5.2 — Objective Conditions
Implement all 23 `condition_species_type` variants from `Objective.h`:
- [x] **P5.2.1** DESTROY group (5): `UObj_DestroyAllEnemy`, `UObj_DestroyNumberOfEnemy`, `UObj_DestroyEnemyGroup`, `UObj_DestroySpecificEnemy`, `UObj_DestroySpecificStructure`.
- [x] **P5.2.2** CAPTURE_OR_DESTROY group (5): all 5 variants implemented in `MC2Objective.cpp`.
- [x] **P5.2.3** DEAD_OR_FLED group (4): `MapBoundaryRadius` circle check via `Size2D()`.
- [x] **P5.2.4** CAPTURE group (2): `CaptureUnit`, `CaptureStructure` — proximity check within `CaptureRadius`.
- [x] **P5.2.5** GUARD group (2): `GuardUnit`, `GuardStructure` — returns Failed when guarded target destroyed.
- [x] **P5.2.6** MOVE group (4): all 4 variants using `ATriggerVolume` → `UShapeComponent::IsOverlappingActor`.
- [x] **P5.2.7** STATE conditions (2): `UObj_BooleanFlagIsSet`, `UObj_ElapsedMissionTime`.
- [ ] **P5.2.8** Objective display in HUD — bind to `UMC2HUDWidget::OnObjectivesUpdated` event (Blueprint implementation).

#### P5.3 — ABL Script Migration (35 missions)
Each `.abl` file becomes a Blueprint Event Graph on a `AMC2MissionScript : AActor` placed in the level. The ABL constructs that map to Blueprint:
- ABL `eternal boolean FlagN` → Blueprint variable (accessible from objective system)
- ABL `triggerArea` → `AMC2MissionVolume` overlap event
- ABL `setObjective` → `GameMode->SetObjectiveState()`
- ABL `playVideo` → Level Sequencer trigger
- ABL `createUnit` → `SpawnActor`
- ABL `timer` → `SetTimer`
- ABL `playSFX` / `playVO` → `UAudioComponent::Play()`
- ABL `setCameraLook` → Level Sequencer camera cut

- [x] **P5.3.1** `MC2MissionScript_Reference.h` — full ABL→Blueprint mapping for `mc2_01.abl`: all 35+ ABL functions, eternal flag index table, tutorial state machine pseudocode, music system, time unit conversion, camera coordinate notes.
- [x] **P5.3.2** `MC2TutorialScript_Reference.h` — logistics tutorial (tutorial0/1/2): screen-ID→widget map, MechBay/PilotSelect/Briefing/MechLab stage machines, rewind rules, UTutorialHelper component approach, VO asset paths.
- [x] **P5.3.3** `MC2CampaignScripts_02_12_Reference.h` — missions 02-12: mission-unique eternal flags, VO/video sequences, objective gating, warrior polling pattern, music tune index table. Notes mc2_03.abl reuses module mc2_12.
- [x] **P5.3.4** `MC2CampaignScripts_13_24_Reference.h` — missions 13-24: eternal flags, VO/video sequences, module name discrepancies (mc2_21=mc2_11, mc2_22=liaoPalace, mc2_23=mc2_07), complete music table, BP class hierarchy summary.
- [x] **P5.3.5** multiplayer.abl: pure music boilerplate (MissionStartTune6, AmbientTune0, random CombatTune0-N); no mission-unique flags, VO, or videos. BP_MC2Mission_MP inherits AMC2GameMode with only music override.

#### P5.4 — Level Construction (24 campaign levels + tutorials)
- [ ] **P5.4.1** Create one level per mission, named `L_MC2_[NN]_[MissionName]`.
- [ ] **P5.4.2** Per level: import/configure Landscape from terrain heightmap, paint Landscape material layers (dirt, rock, grass, water, snow per mission biome).
- [ ] **P5.4.3** Place all static actors: buildings, turrets, trees, props at positions from original mission PAK data.
- [ ] **P5.4.4** Place `AMC2MissionVolume` trigger areas.
- [ ] **P5.4.5** Place `AMC2WaypointActor` patrol path chains for enemy AI.
- [ ] **P5.4.6** Place `AMC2MissionScript` actor and assign the corresponding Blueprint script.
- [ ] **P5.4.7** Set up skybox/lighting per mission biome (day/night, weather).

---

### Phase 6 — Campaign & Logistics
**Goal:** Campaign progression, mech customization, pilot management between missions.

**Duration estimate:** 3–4 weeks

#### P6.1 — Save/Load System
- [x] **P6.1.1** `UMC2SaveGame` — `MechRoster[]`, `PilotRoster[]`, `CompletedMissions[]`, `CBills`, `CampaignFlags[64]`, `TotalPlaytimeSeconds`, slot-based (MC2_Save_N).
- [x] **P6.1.2** Autosave: `CommitMissionToSave()` called from `ApplyMissionResult()` — commits pilot XP, calculates C-Bill rewards, calls `LS->RecordMissionResult / AdvanceToNextMission / SaveGame(0)`.
- [x] **P6.1.3** Save migration — `SaveVersion` int on `UMC2SaveGame` (current=3). `MigrateIfNeeded()` called on every `LoadGame`: v1→v2 fixes zero Gunnery/Piloting, seeds empty `CurrentArmor`; v2→v3 no-op (ComponentInventory defaults empty). Bumps version and resaves after migration.

#### P6.2 — Logistics Subsystem
From `logistics.h`, `LogisticsData.h`:
- [x] **P6.2.1** `UMC2LogisticsSubsystem` — Save/Load/NewCampaign, mech roster CRUD, pilot roster + XP + KIA, mission recording, campaign flags, `OnCBillsChanged` delegate.
- [x] **P6.2.2** C-Bill economy — `SpendCBills()`, `AddCBills()`, `RecordMissionResult()` with CBill reward.
- [x] **P6.2.3** `BeginSalvageSession / AcceptSalvageComponent / CommitSalvage` — point budget (component BattleRating cost), selected components → `CurrentSave->ComponentInventory`. `FMC2MechRecord` now uses `TArray<int32> CurrentArmor` (11 zones).
- [x] **P6.2.4** `RepairMech(RosterIndex, ChassisTable)` — cost = armor damage × 500 C-Bills per point; `SpendCBills` then resets all 11 `CurrentArmor` zones to chassis max. `GetRepairCost` for preview in MechBay HUD.

#### P6.3 — Mech Customization (MechLab)
From `LogisticsVariant.h`, `LogisticsComponent.h`:
- [x] **P6.3.1** `UMC2MechLabValidator::ValidateLoadout` — slot validation per location using standard BT counts (Head=1, CT/LT/RT/LA/RA=12, LL/RL=6). `CanAddComponentToLocation` checks `FMC2ComponentRow::LocXxx` bools + slot budget.
- [x] **P6.3.2** `ValidateLoadout` also checks total weight ≤ `ChassisRow.Tonnage`; returns `FMC2ValidationResult` with weight breakdown and per-location slot usage.
- [x] **P6.3.3** `LoadFactoryVariant(ChassisID, VariantIndex)` → reads `DT_MechVariants` row `ChassisID_N`, converts Item0-19 to ComponentSlots. `SaveCustomVariant` / `ResetToFactoryVariant` on LogisticsSubsystem.
- [x] **P6.3.4** Implement tech base: Clan vs Inner Sphere components (Clan = smaller/lighter, IS = heavier but available earlier in campaign). `EMC2TechBase` enum on `FMC2ComponentRow`. `IsComponentAvailable(row, bClanTechUnlocked)` + `FilterAvailableComponents()` on validator. `UnlockClanTech()` / `IsClanTechUnlocked()` on LogisticsSubsystem (reads CampaignFlag[32], set by mission 13).

#### P6.4 — Pilot System
From `LogisticsPilot.h`, `MechWarrior.h`:
- [x] **P6.4.1** `UMC2PilotComponent` — `Gunnery/Piloting` (lower=better), `InjuryLevel`, 2d6 `HitTable[]`, `RollToHit(Range)`, `RollPilotingCheck(Modifier)`, `CommitXPToSave()` → `LS->AwardPilotXP/KillPilot`.
- [x] **P6.4.2** `AwardPilotXP`: every 500 XP improves Gunnery (even tiers) or Piloting (odd tiers) by 1, min 2. `FMC2PilotRecord` extended with `Gunnery/Piloting` fields (saved per-pilot, start from DT_Pilots values).
- [x] **P6.4.3** Pilot KIA: `UMC2PilotComponent::TakeInjury(4)` → `OnPilotKilled`, then `CommitXPToSave` calls `LS->KillPilot(PilotID)` → `bAlive=false` in save.
- [x] **P6.4.4** Implement pilot assignment: drag-assign pilots to mechs in MechBay. `AssignPilotToMech` validates pilot is alive + auto-clears prior mech assignment. `UnassignPilot(PilotID)` + `GetMechIndexForPilot(PilotID)` for UI bidirectional binding.

---

### Phase 7 — UI (UMG)
**Goal:** Complete UI matching MC2's screens. FIT files are text INI format, so layout data is accessible.

**Duration estimate:** 4–5 weeks (UI is always underestimated)

- [ ] **P7.1** Create `WBP_MainMenu`: mission select, load game, options, quit. Use FIT layout from `mainscreen.fit` as reference for element positions.
- [x] **P7.2** `AMC2HUD` + `UMC2HUDWidget` + `UMC2UnitPortraitWidget` + `UMC2MinimapWidget` — rubber-band selection box, armor/heat bars (traffic-light color), minimap blips, mission clock, C-Bills display. Remaining: UMG designer layout in Blueprint.
- [ ] **P7.3** Create `WBP_TacMap`: full-screen tactical map overlay, shows unit positions, nav markers, fog of war.
- [ ] **P7.4** Create `WBP_MechBay`: mech roster list, pilot assignment, repair queue, C-Bill balance.
- [ ] **P7.5** Create `WBP_MechLab`: drag-and-drop component loadout editor, weight/slot validation display, armor allocation sliders.
- [ ] **P7.6** Create `WBP_PilotReady`: pre-mission lance assignment, pilot/mech pairing.
- [ ] **P7.7** Create `WBP_MissionBriefing`: mission text, objectives list, map overview.
- [ ] **P7.8** Create `WBP_MissionResults`: success/failure, kills/losses summary, salvage selection, XP gained.
- [ ] **P7.9** Create `WBP_Salvage`: drag salvaged components to inventory.
- [ ] **P7.10** Create `WBP_Loading`: loading screen with mission briefing text, progress bar.
- [ ] **P7.11** Create `WBP_Options`: graphics, audio, keybinding settings. Store in `UGameUserSettings`.
- [ ] **P7.12** Create `WBP_MPLobby`: multiplayer session browser, host/join, lance selection (from `MCL_MP_*.fit` files).

---

### Phase 8 — Multiplayer
**Goal:** LAN/online co-op and skirmish matching MC2's multiplayer.

**Duration estimate:** 3–4 weeks

MC2 used direct TCP/IP via `MPDirectTcpip`. UE5 replaces this entirely with its built-in replication system.

- [x] **P8.1** `AMC2Mover` has `bReplicates`, `DOREPLIFETIME` for `ArmorZones`, `CurrentHeat`, `TeamIndex`, `bIsDestroyed`, `bShutDown`, `bLegsDamaged`.
- [x] **P8.2** Move orders go through `ReceiveMoveOrder` on server (PlayerController → Mover); AI controller executes via NavMesh.
- [x] **P8.3** Weapon fire and damage applied server-side; `OnRep_ArmorZones` propagates to clients.
- [x] **P8.4** `UMC2SessionSubsystem` — `HostSession`, `FindSessions`, `JoinFoundSession`, `DestroySession` with OSS delegate binding. Supports NULL (LAN) and Steam OSS. `OnSessionCreated/Joined/Found/Destroyed` Blueprint delegates.
- [x] **P8.5** `AMC2GameState` fully replicated: `MissionResult` (OnRep), `ScenarioTimers[8]`, `MissionFlags[64]`, `Teams[]` (FMC2TeamState).
- [x] **P8.6** Mission sync: `Multicast_OnMissionReady(TotalUnits)` on `AMC2GameState` fired from `AMC2GameMode::StartPlay()` after `SpawnAllParts()`; late-join handled by `Server_RequestMissionSync` RPC on PlayerController.
- [x] **P8.7** `UMC2LobbySubsystem` — `FMC2LobbyPlayer` (ID, name, team, bReady, LanceMechs[4]). `SetSelectedMap/SetTeamSize` (host), `SetLocalPlayerReady/Lance/Team`, `RegisterPlayer/UnregisterPlayer/UpdatePlayerState`. `AreAllPlayersReady()` + `StartMatch()` → `ServerTravel(MissionID?listen)`. `OnLobbyStateChanged/OnMatchStarting` delegates drive `WBP_MPLobby`.

---

### Phase 9 — Polish & Platform
**Goal:** Performance, settings, final content pass.

**Duration estimate:** 2–3 weeks

- [ ] **P9.1** LOD setup for all Skeletal Meshes: 3 LOD levels per mech at 100/300/600m.
- [ ] **P9.2** Occlusion culling configuration for open terrain levels.
- [ ] **P9.3** Post-process volume: slight vignette, contrast boost to match MC2's look. Optional film grain toggle.
- [ ] **P9.4** Level streaming: load/unload terrain sectors as camera moves (for large 120×120 maps).
- [ ] **P9.5** Performance target: 60fps at 1080p with 20 units on screen. Profile and fix any bottlenecks.
- [ ] **P9.6** keybinding system: support remapping all actions via `UEnhancedInputComponent`.
- [ ] **P9.7** Accessibility: subtitle system for all VO/dialogue, colorblind mode (replace red/green UI indicators with shapes).
- [ ] **P9.8** Final content pass: verify all 24 missions play through correctly, objectives fire, win/lose correctly.

---

## Critical Path & Risk Register

### Critical Path (no slippage allowed)

```
P0.3 (3D Model Extraction) → P2.2 (BattleMech Blueprint) → P2.4 (Selection & Orders)
         ↓                           ↓
P5.4 (Level Construction) → P5.3 (ABL Script Migration) → Playable campaign
```

### Risk Register

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| 3D model binary format unreadable | Medium | Critical | Find original ASE files first; if not found, re-model key mechs from reference images |
| TXM extractor produces corrupted output | Low | High | Cross-check against TGA source files (many textures exist in both formats) |
| ABL scripts use undocumented engine calls | Medium | Medium | MC2 ABL functions are defined in `Source/MCLib/Abl*.cpp` — all are catalogued |
| UE NavMesh doesn't match MC2 movement | Low | Medium | Custom `UNavMovementComponent` fallback |
| 35 ABL missions × avg 480 lines = significant Blueprint work | High (effort risk) | Medium | Consider a UE Blueprint-callable ABL interpreter as alternative to manual migration |
| UE multiplayer replication latency vs original TCP/IP | Low | Low | UE networking is superior; original was LAN-only |

---

## Total Effort Estimate

| Phase | Duration |
|---|---|
| Phase 0 — Asset Pipeline | 2–4 weeks |
| Phase 1 — UE Project Skeleton | 1–2 weeks |
| Phase 2 — Core Units | 3–4 weeks |
| Phase 3 — Combat | 3–4 weeks |
| Phase 4 — AI & Pathfinding | 3–4 weeks |
| Phase 5 — Mission System | 4–5 weeks |
| Phase 6 — Campaign & Logistics | 3–4 weeks |
| Phase 7 — UI | 4–5 weeks |
| Phase 8 — Multiplayer | 3–4 weeks |
| Phase 9 — Polish | 2–3 weeks |
| **Total (1 developer)** | **28–39 weeks (7–10 months)** |
| **Total (2 developers)** | **16–22 weeks (4–5 months)** |

The biggest accelerator: finding the original 3DS Max `.ase` source files for 3D models. That alone could save 4–6 weeks on P0.3.

---

## First Week Checklist (Start Here)

1. [ ] Check `Source/Data/TGL/` directory for any `.ase` or `.max` files — this determines the model extraction strategy.
2. [ ] Run a batch import of all `.tga` textures from `Source/Data/Textures/` into a new UE5 project.
3. [ ] Run a batch import of all `.wav` audio files into the UE5 project.
4. [ ] Write and test the TXM extractor Python script (see P0.1 above).
5. [ ] Set up the FIT parser (P0.2) and verify one mech definition (e.g. `AWESOME1.FIT`) converts correctly to a Data Table row.
6. [ ] Create UE5 project with top-down camera and placeholder landscape.
