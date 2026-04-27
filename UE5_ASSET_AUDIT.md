# MC2 → UE5 Asset Audit

Full mapping of every file type and directory in the original repo to its
fate in the UE5 project. Status: ✅ handled | ⚠️ partial | ❌ not handled | ➖ not needed

---

## Source/Data/Art/ — 663 files (527 TGA, 135 FIT, 1 CSV)

| Files | What they are | UE5 status |
|---|---|---|
| `mcl_*.tga` | UI artwork (menu backgrounds, buttons, HUD panels) | ✅ `ue_import_textures.py` → `/Game/Textures/UI/` |
| `mcl_*.fit` | UI screen layout definitions (positions, animations, button IDs) | ✅ Read to write all 20 widget C++ backends (P7.1–P7.20). Not imported as assets — they drove the C++ design. |
| `*.csv` | Single row struct file | ✅ Covered by `fit_convert.py` data table output |

---

## Source/Data/TGL/ — 7,638 files (2,947 ASE, 2,803 ARM, 1,114 TGA)

| Files | What they are | UE5 status |
|---|---|---|
| `*.ase` | 3D models: mechs, vehicles, buildings, props, animations | ✅ `ase_to_fbx.py` → 2,927 FBX → `ue_import_meshes.py` + `ue_import_animations.py` |
| `*.arm` (XML) | Asset Relationship Metadata — maps which FIT/CSV/INI files belong to each model. Editor tooling only. | ➖ Not needed at runtime. No UE equivalent required. |
| `*.tga` | Texture maps for the 3D models | ✅ `ue_import_textures.py` → `/Game/Textures/Mechs/` etc. |
| `*.ini` | Per-object config (scale, pivot, classification hints) | ✅ Read by `extract_all_mission_data.py` to classify mechs/vehicles/buildings |

---

## Source/Data/Textures/ — 1,396 files (1,214 TGA, 100 TXM, 46 JPG)

| Files | What they are | UE5 status |
|---|---|---|
| `*.txm` | Compressed runtime textures (custom LZW format) | ✅ `txm_extract.py` → 100 PNGs → `ue_import_txm_pngs.py` → `/Game/Textures/TXM/` |
| `64/*.tga` | 64×64 terrain/UI tiles | ✅ `ue_import_textures.py` |
| `64Mask/*.tga` | Alpha mask tiles for terrain blending | ✅ `ue_import_textures.py` |
| `64Overlays/*.tga` | Terrain overlay textures | ✅ `ue_import_textures.py` |
| `Defaults/*.tga` | Fallback textures | ✅ `ue_import_textures.py` |
| `Random_Maps/*.tga` | Procedural/skirmish map textures | ✅ `ue_import_textures.py` |
| `*.jpg` | Reference/preview images (not used at runtime) | ➖ Not needed |

---

## Source/Data/Sound/ — 1,807 files (1,770 WAV, 33 RSP, 2 PAK)

| Files | What they are | UE5 status |
|---|---|---|
| `*.wav` | All game audio: weapons, ambient, music, pilot VO, UI | ✅ `ue_import_audio.py` → `/Game/Audio/` with Sound Class hierarchy |
| `*.rsp` | Random sound lists — each RSP is a text file listing WAV filenames to pick from randomly | ⚠️ `ue_build_sound_cues.py` groups variants by base name but does not read RSP files directly. **Gap: RSP files should be parsed to build SoundNodeRandom cues with the exact MC2 variant groupings.** |
| `*.pak` | Compressed sound archives (2 files: `mc2_m02a.pak`, `mc2_m08.pak`) | ❌ Binary format, not unpacked. Likely contain the same WAVs already present individually. Low risk — verify nothing is missing from the individual WAV set. |

---

## Source/Data/Missions/ — 3,737 files (2,394 ARM, 991 ABL, 274 FIT)

| Files | What they are | UE5 status |
|---|---|---|
| `mc2_NN.fit` | Mission layout: warrior/part placements, terrain, weather, objectives | ✅ `extract_all_mission_data.py` → 24 JSON files → `ue_build_mission.py` |
| `*.abl` | AI/trigger scripts per mission (patrol logic, objectives, cinematics) | ⚠️ `extract_all_mission_data.py` parses trigger areas and patrol chains from ABL. Full ABL→Blueprint migration documented in `MC2MissionScript_Reference.h` and `MC2TutorialScript_Reference.h`. **Each mission still needs its Blueprint Event Graph built manually.** |
| Warriors/`*.fit` | Per-warrior AI profiles (skill, brain assignment) | ✅ Read by `extract_all_mission_data.py` for patrol chain extraction |
| Warriors/`*.abl` | Per-group AI brain scripts | ✅ Parsed for patrol waypoint chains |
| Profiles/`*.fit` | Warrior stat profiles | ✅ Feeds into `DT_Pilots` data table via `fit_convert.py` |
| `*.arm` (XML) | Asset relationship metadata for mission assets | ➖ Editor tooling only, not needed at runtime |
| `*.pak` | Compiled mission packages (5 files) | ❌ Binary Mach-O format — likely pre-compiled mission data superseded by the source FIT/ABL files. Safe to skip. |

---

## Source/Data/Objects/ — 1,241 files (1,196 FIT, 41 CSV, 2 PAK)

| Files | What they are | UE5 status |
|---|---|---|
| `*.fit` | Unit/building definitions: stats, weapon loadouts, armor values | ✅ `fit_convert.py` → 6 CSV files → `ue_import_data_tables.py` → DT_MechChassis, DT_MechVariants, DT_VehicleTypes, DT_Components, DT_BuildingTypes, DT_Pilots |
| `*.csv` | Unit stat overrides (some units use CSV instead of FIT) | ✅ Included in `fit_convert.py` processing |
| `feet.pak`, `object2.pak` | Compiled object packages | ❌ Binary format. Source FITs/CSVs contain all the same data. Safe to skip. |

---

## Source/Data/terrain/ — 4 TGA files

| Files | What they are | UE5 status |
|---|---|---|
| `60x60.tga`, `80x80.tga`, `100x100.tga`, `120x120.tga` | Heightmap templates at different mission sizes | ✅ Referenced by `ue_build_mission.py` for landscape import. Actual per-mission heightmaps are derived from these + mission FIT data. |

---

## Source/Data/Cameras/ — 2 FIT files

| Files | What they are | UE5 status |
|---|---|---|
| `Cameras.fit` | RTS camera parameters: zoom limits, pan speed, tilt angle | ⚠️ Values used to configure `AMC2CameraActor`. **Not yet imported as a data asset** — camera params are currently hardcoded in `MC2CameraActor.cpp`. Low priority. |
| `Colors.fit` | Team colour palette (8 teams) | ⚠️ Team colours referenced in `FMC2TeamState` and scoreboard widgets but not imported as a data table. Currently hardcoded. Low priority. |

---

## Source/Data/Effects/ — 1 FX file

| Files | What they are | UE5 status |
|---|---|---|
| `*.fx` | Binary GOSFX particle effect definitions (explosions, muzzle flash, etc.) | ❌ Proprietary binary format (GOSFX engine). No parser written. **UE5 replacement:** Niagara effect assets need to be created manually in the editor. `MC2FXLibrary.cpp` defines the C++ hooks (spawn points, scale, team colour). The original effect names are: Fireball, Missile_Flare, Gauss_flare, star — use these as reference when building Niagara systems. |

---

## Source/Data/Movies/ — 32 TGA files

| Files | What they are | UE5 status |
|---|---|---|
| `Minelaye.tga`, `bubba.tga`, etc. | Still-frame thumbnails/posters for FMV cutscenes | ✅ `ue_import_textures.py` will catch these |
| BIK video files | Full-motion video cutscenes (not present in repo — likely too large for git) | ❌ Not present in source repo. If you have the original game disc, BIK files can be converted to MP4 and imported via UE5 Media Framework. |

---

## Source/Data/Campaign/ — 18 files (16 ARM, 2 FIT)

| Files | What they are | UE5 status |
|---|---|---|
| `campaign.fit` | Campaign progression: 13 mission groups, C-Bill starting budget (80,000), unlock order, video assignments | ⚠️ Structure is captured in `MC2LogisticsSubsystem.cpp` (group/mission unlock logic) but **not imported as a DataTable**. Should become `DT_Campaign` with one row per mission group. |
| `tutorial.fit` | Tutorial mission sequence and stage definitions | ⚠️ Referenced in `MC2TutorialScript_Reference.h` but not a DataTable asset. |
| `*.arm` (XML) | Asset metadata | ➖ Not needed |

---

## Source/Data/MultiPlayer/ — 55 TGA files

| Files | What they are | UE5 status |
|---|---|---|
| `Insignia/icon*.tga` | Team/faction badge icons for MP lobby | ✅ `ue_import_textures.py` → `/Game/Textures/UI/` |

---

## Source/Code/ — 209 files (116 H, 93 CPP)

| Files | What they are | UE5 status |
|---|---|---|
| `Actor.*`, `Mover.*`, `BattleMech.*` etc. | Original MC2 game C++ — unit logic, AI, GUI, logistics | ✅ **Read as reference** to write all UE5 C++ equivalents. Not imported. Replaced by `UE5Project/Source/MechCommander2/`. |

---

## Source/MCLib/ — 498 files (238 CPP, 149 HPP, 104 H)

| Files | What they are | UE5 status |
|---|---|---|
| `GOSFX/` | Particle effect engine | ➖ Replaced by Niagara |
| `MLR/` | Custom 3D renderer | ➖ Replaced by UE5 renderer |
| `Stuff/` | Math/utility library | ➖ Replaced by UE5 math library |
| `Abl*.cpp` | ABL scripting language interpreter | ➖ Replaced by Blueprint. Read to understand ABL semantics. |
| `mech3d.h` | 25 gesture constants, bone names | ✅ Referenced to write `ABP_BattleMech_Reference.h` and `ue_import_animations.py` |

---

## Source/GameOS/ — 76 files

| Files | What they are | UE5 status |
|---|---|---|
| All | Platform abstraction layer (input, audio, network, file I/O) | ➖ Entirely replaced by UE5 engine. Not needed. |

---

## Source/GUI/ — 19 files

| Files | What they are | UE5 status |
|---|---|---|
| `aAnimObject`, `aButton`, `aFont`, `aListBox` etc. | Original FIT-driven UI widget system | ✅ **Read as reference** to understand FIT animation format and widget behaviour. Replaced by UE5 UMG + our 20 widget C++ backends. |

---

## Source/ABLT/ — 5 files

| Files | What they are | UE5 status |
|---|---|---|
| ABL compiler source | Tokenizer and compiler for the ABL scripting language | ✅ **Read as reference** to understand ABL syntax. Replaced by Blueprint. |

---

## FinalBuild/ — compiled game data

Mirrors Source/Data/ but in compiled/packed form. Where both exist, the Source version was used (it's unpacked and readable). Unique items:

| Files | What they are | UE5 status |
|---|---|---|
| `data/savegame/` | Save game slot format | ✅ Structure referenced to write `MC2SaveGame` and `MC2LogisticsSubsystem` save/load system |
| `art.fst`, `camera.fst` | File system table (packed asset manifest) | ➖ Not needed — we use the unpacked source files |
| `*.bik` | BIK video cutscenes | ❌ Not present in repo (see Movies above) |

---

## Summary of Gaps

| Gap | Severity | Fix |
|---|---|---|
| **ABL mission scripts** — 24 missions need Blueprint Event Graphs | High | Manual work per mission using `MC2MissionScript_Reference.h` as guide |
| **Niagara FX** — 4 effect types (Fireball, Missile_Flare, Gauss_flare, star) | High | Create Niagara systems in editor; `MC2FXLibrary.cpp` provides spawn hooks |
| **BIK cutscene videos** — not in repo | Medium | Source from original disc; convert to MP4; import via UE5 Media Framework |
| **RSP random sound files** — 33 files not parsed | Low | Extend `ue_build_sound_cues.py` to read RSP and build exact variant groupings |
| **campaign.fit / tutorial.fit** — not a DataTable | Low | Add `DT_Campaign` to `fit_convert.py` output and `ue_import_data_tables.py` |
| **WBP visual layouts** — all 20 widgets need UMG designer work | Medium | Manual work in UE editor per widget |
| **Cameras.fit / Colors.fit** — hardcoded in C++ | Low | Optional: import as DataTable for designer-tweakable values |
