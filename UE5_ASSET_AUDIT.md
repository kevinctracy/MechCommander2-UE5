# MechCommander 2 → Unreal Engine 5 Asset Audit

Full mapping of every file type and directory in the original source repository to its
fate in the Unreal Engine 5 project. Status: ✅ handled | ⚠️ partial | ❌ not handled | ➖ not needed

### File format glossary
- **TGA** — Truevision Graphics Adapter image. Standard lossless texture format used throughout MC2.
- **FIT** — MC2's own INI-style configuration format. Not an acronym — just the file extension used for everything from UI layouts to unit definitions.
- **CSV** — Comma-Separated Values. Spreadsheet-style data file.
- **ASE** — ASCII Scene Export. 3DS Max's plain-text 3D model format. Every mesh and animation in the game is stored this way.
- **FBX** — Filmbox. Autodesk's standard 3D interchange format that Unreal Engine imports natively. We convert ASE → FBX as an intermediate step.
- **ARM** — Asset Relationship Metadata. MC2 editor's proprietary XML format that records which files belong together (e.g. which texture goes with which model). Runtime irrelevant.
- **INI** — Initialization file. Plain-text key=value config, used per-object in the TGL folder.
- **TXM** — MC2's compressed runtime texture format (custom LZW compression). Must be extracted to PNG before Unreal can use them.
- **LZW** — Lempel–Ziv–Welch. A lossless data compression algorithm. MC2 uses a variable-width variant for TXM files.
- **PNG** — Portable Network Graphics. Lossless image format used as the intermediate step between TXM and Unreal.
- **WAV** — Waveform Audio File Format. Uncompressed audio. All MC2 sound effects and voice-over are WAV files.
- **RSP** — Response file. MC2's plain-text lists of WAV filenames to pick from at random (e.g. multiple "acknowledge" lines for a pilot).
- **PAK** — Package file. Binary compressed archive. MC2 uses these to bundle compiled mission or object data.
- **ABL** — MC2's proprietary scripting language used to drive AI behaviour, mission triggers, and objectives. Stands for nothing specific — it's the language name.
- **BIK** — Bink Video. RAD Game Tools' proprietary compressed video format used for MC2's full-motion video cutscenes.
- **FX** — Effects file. Binary particle effect definitions used by GOSFX, MC2's particle engine.
- **GOSFX** — Graphics Object System Effects. Microsoft's proprietary particle effect engine embedded in the GameOS layer.
- **MLR** — Microsoft's proprietary 3D rendering library used under the hood of MC2's engine.
- **FST** — File System Table. A binary manifest listing all packed assets in the shipped game.
- **JSON** — JavaScript Object Notation. Human-readable data format we use as the intermediate output of our mission extraction scripts.
- **Unreal Engine / UE5** — Unreal Engine 5. The game engine we are migrating MC2 into.
- **Blueprint** — Unreal Engine's visual scripting system. Replaces ABL for mission logic and gameplay scripting.
- **Unreal Motion Graphics (UMG)** — Unreal Engine's user interface designer. Replaces MC2's FIT-driven GUI system.
- **Widget Blueprint (WBP)** — A UMG Blueprint asset. The visual layout half of each UI screen. Our C++ classes are the logic half.
- **Niagara** — Unreal Engine's particle and visual effects system. Replaces GOSFX.
- **Data Table (DT_)** — An Unreal Engine asset that stores rows of structured data, equivalent to a spreadsheet. Used for mech stats, pilot data, etc.
- **Skeletal Mesh (SK_)** — An Unreal Engine 3D mesh asset with a bone skeleton, used for animated characters like mechs.
- **Static Mesh (SM_)** — An Unreal Engine 3D mesh asset with no skeleton, used for buildings and props.
- **Animation Blueprint (ABP_)** — An Unreal Engine asset that drives skeletal mesh animations using a state machine.
- **Physics Asset (PA_)** — An Unreal Engine asset defining collision shapes and physical properties for a skeletal mesh.
- **Heads-Up Display (HUD)** — The in-game overlay showing health bars, minimap, C-Bills, and mission clock.
- **Voice-Over (VO)** — Spoken audio lines from pilots and tactical officers.
- **Full-Motion Video (FMV)** — Pre-rendered cinematic video cutscenes.
- **Level of Detail (LOD)** — Lower-polygon versions of a mesh used at distance to save GPU performance.
- **Real-Time Strategy (RTS)** — The genre MC2 belongs to — overhead camera, unit selection, issuing orders.
- **Artificial Intelligence (AI)** — The computer-controlled enemy behaviour system.
- **C-Bills** — The in-game currency used to repair mechs and buy components.

---

## Source/Data/Art/ — 663 files (527 Truevision Graphics Adapter images, 135 FIT layouts, 1 Comma-Separated Values file)

| Files | What they are | Unreal Engine 5 status |
|---|---|---|
| `mcl_*.tga` | User interface artwork — menu backgrounds, buttons, Heads-Up Display panels | ✅ `ue_import_textures.py` → `/Game/Textures/UI/` |
| `mcl_*.fit` | User interface screen layout definitions — positions, animations, button identifiers for every menu screen | ✅ Read to write all 20 widget C++ backends (phases 7.1–7.20). Not imported as assets — they drove the C++ design. |
| `*.csv` | Single data row file | ✅ Covered by `fit_convert.py` data table output |

---

## Source/Data/TGL/ — 7,638 files (2,947 ASCII Scene Export, 2,803 Asset Relationship Metadata, 1,114 Truevision Graphics Adapter)

| Files | What they are | Unreal Engine 5 status |
|---|---|---|
| `*.ase` | 3D models: mechs, vehicles, buildings, props, and all animations | ✅ `ase_to_fbx.py` converts to 2,927 Filmbox files → `ue_import_meshes.py` imports meshes, `ue_import_animations.py` imports animations |
| `*.arm` (XML) | Asset Relationship Metadata — maps which FIT, Comma-Separated Values, and INI files belong to each model. Used by the original MC2 editor only. | ➖ Not needed at runtime. No Unreal Engine equivalent required. |
| `*.tga` | Texture maps painted onto the 3D models | ✅ `ue_import_textures.py` → `/Game/Textures/Mechs/` etc. |
| `*.ini` | Per-object config files recording scale, pivot point, and classification hints | ✅ Read by `extract_all_mission_data.py` to classify each object as mech, vehicle, or building |

---

## Source/Data/Textures/ — 1,396 files (1,214 Truevision Graphics Adapter, 100 TXM compressed textures, 46 JPEG images)

| Files | What they are | Unreal Engine 5 status |
|---|---|---|
| `*.txm` | MC2's compressed runtime textures using custom Lempel–Ziv–Welch compression | ✅ `txm_extract.py` extracts 100 Portable Network Graphics files → `ue_import_txm_pngs.py` imports them into `/Game/Textures/TXM/` |
| `64/*.tga` | 64×64 pixel terrain and user interface tiles | ✅ `ue_import_textures.py` |
| `64Mask/*.tga` | Alpha mask tiles used to blend terrain layers | ✅ `ue_import_textures.py` |
| `64Overlays/*.tga` | Overlay textures painted on top of terrain | ✅ `ue_import_textures.py` |
| `Defaults/*.tga` | Fallback textures shown when a specific texture is missing | ✅ `ue_import_textures.py` |
| `Random_Maps/*.tga` | Textures for procedurally generated skirmish maps | ✅ `ue_import_textures.py` |
| `*.jpg` | Reference and preview images not used at runtime | ➖ Not needed |

---

## Source/Data/Sound/ — 1,807 files (1,770 Waveform Audio, 33 Response files, 2 Package archives)

| Files | What they are | Unreal Engine 5 status |
|---|---|---|
| `*.wav` | All game audio: weapon sounds, ambient noise, music, pilot voice-over, user interface clicks | ✅ `ue_import_audio.py` → `/Game/Audio/` with Sound Class hierarchy for volume control |
| `*.rsp` | Random sound lists — each file is a plain-text list of Waveform Audio filenames. MC2 picks one at random each time that sound is needed (e.g. three different "acknowledged" lines for a pilot) | ⚠️ `ue_build_sound_cues.py` groups variants by base name but does not read Response files directly. **Gap: these files should be parsed to build Unreal's SoundNodeRandom cues with the exact same variant groupings MC2 used.** |
| `*.pak` | Compressed sound archives (2 files) | ❌ Binary format, not unpacked. Likely contain the same Waveform Audio files already present individually. Low risk — verify nothing is missing from the individual file set. |

---

## Source/Data/Missions/ — 3,737 files (2,394 Asset Relationship Metadata, 991 ABL scripts, 274 FIT layouts)

| Files | What they are | Unreal Engine 5 status |
|---|---|---|
| `mc2_NN.fit` | Mission layout files: warrior and part placements, terrain type, weather, objectives | ✅ `extract_all_mission_data.py` → 24 JSON files → `ue_build_mission.py` builds all 24 Unreal Engine levels |
| `*.abl` | Artificial Intelligence and trigger scripts per mission — patrol routes, objective conditions, cinematics, unit spawns | ⚠️ `extract_all_mission_data.py` parses trigger areas and patrol chains. Full ABL→Blueprint migration is documented in `MC2MissionScript_Reference.h` and `MC2TutorialScript_Reference.h`. **Each mission still needs its Blueprint Event Graph built manually in the editor.** |
| Warriors/`*.fit` | Per-warrior Artificial Intelligence profiles — skill level and brain assignment | ✅ Read by `extract_all_mission_data.py` for patrol chain extraction |
| Warriors/`*.abl` | Per-group Artificial Intelligence brain scripts | ✅ Parsed for patrol waypoint chains |
| Profiles/`*.fit` | Warrior stat profiles | ✅ Feeds into the `DT_Pilots` Data Table via `fit_convert.py` |
| `*.arm` (XML) | Asset Relationship Metadata for mission assets | ➖ Editor tooling only, not needed at runtime |
| `*.pak` | Compiled mission package files (5 files) | ❌ Binary format — likely pre-compiled mission data superseded by the source FIT and ABL files. Safe to skip. |

---

## Source/Data/Objects/ — 1,241 files (1,196 FIT definitions, 41 Comma-Separated Values, 2 Package archives)

| Files | What they are | Unreal Engine 5 status |
|---|---|---|
| `*.fit` | Unit and building definitions: stats, weapon loadouts, armor values, weight class | ✅ `fit_convert.py` → 6 Comma-Separated Values files → `ue_import_data_tables.py` → Data Tables: `DT_MechChassis` (mech frames), `DT_MechVariants` (specific loadouts), `DT_VehicleTypes`, `DT_Components` (weapons and equipment), `DT_BuildingTypes`, `DT_Pilots` |
| `*.csv` | Unit stat overrides — some units store stats directly in Comma-Separated Values instead of FIT | ✅ Included in `fit_convert.py` processing |
| `feet.pak`, `object2.pak` | Compiled object package archives | ❌ Binary format. The source FIT and Comma-Separated Values files contain all the same data. Safe to skip. |

---

## Source/Data/terrain/ — 4 Truevision Graphics Adapter heightmap files

| Files | What they are | Unreal Engine 5 status |
|---|---|---|
| `60x60.tga`, `80x80.tga`, `100x100.tga`, `120x120.tga` | Heightmap templates at four mission sizes (measurements in tiles) | ✅ Referenced by `ue_build_mission.py` for landscape import. Each mission's actual terrain is derived from one of these templates combined with the mission FIT data. |

---

## Source/Data/Cameras/ — 2 FIT files

| Files | What they are | Unreal Engine 5 status |
|---|---|---|
| `Cameras.fit` | Real-Time Strategy camera parameters: zoom limits, pan speed, tilt angle | ⚠️ Values used to configure `AMC2CameraActor`. **Not yet imported as a data asset** — camera parameters are currently hardcoded in `MC2CameraActor.cpp`. Low priority. |
| `Colors.fit` | Team colour palette for all 8 teams | ⚠️ Team colours are referenced in `FMC2TeamState` and scoreboard widgets but not imported as a Data Table. Currently hardcoded in C++. Low priority. |

---

## Source/Data/Effects/ — 1 FX binary file

| Files | What they are | Unreal Engine 5 status |
|---|---|---|
| `*.fx` | Binary GOSFX (Graphics Object System Effects) particle effect definitions — explosions, muzzle flash, engine exhaust | ❌ Proprietary binary format tied to the GOSFX engine. No parser written. **Unreal Engine 5 replacement:** Niagara particle effect assets need to be created manually in the editor. `MC2FXLibrary.cpp` defines the C++ spawn hooks. The original effect names — Fireball, Missile_Flare, Gauss_flare, star — should be used as the reference names when building Niagara systems. |

---

## Source/Data/Movies/ — 32 Truevision Graphics Adapter stills

| Files | What they are | Unreal Engine 5 status |
|---|---|---|
| `Minelaye.tga`, `bubba.tga`, etc. | Still-frame thumbnail posters for each Full-Motion Video cutscene | ✅ `ue_import_textures.py` will pick these up |
| Bink Video files (not present) | Full-Motion Video cutscenes — the actual in-game cinematics | ❌ Not present in the source repository (too large for version control). If you have the original game disc, Bink Video files can be converted to MP4 and imported via Unreal Engine's Media Framework. |

---

## Source/Data/Campaign/ — 18 files (16 Asset Relationship Metadata, 2 FIT files)

| Files | What they are | Unreal Engine 5 status |
|---|---|---|
| `campaign.fit` | Campaign progression: 13 mission groups, starting C-Bills budget (80,000), mission unlock order, video assignments | ⚠️ The structure is captured in `MC2LogisticsSubsystem.cpp` but **not imported as a Data Table**. Should become `DT_Campaign` with one row per mission group so designers can adjust unlock order without recompiling. |
| `tutorial.fit` | Tutorial mission sequence and stage definitions | ⚠️ Referenced in `MC2TutorialScript_Reference.h` but not yet a Data Table asset. |
| `*.arm` (XML) | Asset Relationship Metadata | ➖ Not needed |

---

## Source/Data/MultiPlayer/ — 55 Truevision Graphics Adapter files

| Files | What they are | Unreal Engine 5 status |
|---|---|---|
| `Insignia/icon*.tga` | Team and faction badge icons shown in the multiplayer lobby | ✅ `ue_import_textures.py` → `/Game/Textures/UI/` |

---

## Source/Code/ — 209 files (116 header files, 93 C++ source files)

| Files | What they are | Unreal Engine 5 status |
|---|---|---|
| `Actor.*`, `Mover.*`, `BattleMech.*` etc. | The original MC2 game C++ source — unit logic, Artificial Intelligence, user interface, logistics | ✅ **Read as reference** to write all Unreal Engine 5 C++ equivalents. Not imported. Replaced entirely by `UE5Project/Source/MechCommander2/`. |

---

## Source/MCLib/ — 498 files (238 C++ source, 149 C++ headers, 104 C headers)

| Files | What they are | Unreal Engine 5 status |
|---|---|---|
| `GOSFX/` | Graphics Object System Effects — MC2's proprietary particle engine | ➖ Replaced by Niagara |
| `MLR/` | Microsoft's proprietary 3D rendering library | ➖ Replaced by Unreal Engine's renderer |
| `Stuff/` | Math and utility library | ➖ Replaced by Unreal Engine's math library |
| `Abl*.cpp` | ABL scripting language interpreter — the runtime that executed mission scripts | ➖ Replaced by Blueprint. Read to understand ABL semantics when migrating scripts. |
| `mech3d.h` | Defines 25 gesture constants (Park, Walk, Run, Jump, etc.) and bone names | ✅ Referenced to write `ABP_BattleMech_Reference.h` and to build the gesture-to-animation mapping in `ue_import_animations.py` |

---

## Source/GameOS/ — 76 files

| Files | What they are | Unreal Engine 5 status |
|---|---|---|
| All files | Platform abstraction layer — wraps input, audio, networking, and file input/output for Windows | ➖ Entirely replaced by Unreal Engine. Not needed. |

---

## Source/GUI/ — 19 files

| Files | What they are | Unreal Engine 5 status |
|---|---|---|
| `aAnimObject`, `aButton`, `aFont`, `aListBox` etc. | MC2's original FIT-driven user interface widget system — the C++ classes that read FIT layout files and rendered buttons, text, and animations | ✅ **Read as reference** to understand how FIT animation timing and widget IDs work. Replaced by Unreal Motion Graphics + 20 widget C++ backends. |

---

## Source/ABLT/ — 5 files

| Files | What they are | Unreal Engine 5 status |
|---|---|---|
| ABL compiler source | Tokenizer and compiler that converted ABL script source into bytecode | ✅ **Read as reference** to understand ABL syntax and function names. Replaced by Blueprint. |

---

## FinalBuild/ — compiled and shipped game data

Mirrors Source/Data/ but in compiled and packed form. Where both exist, the Source version was used because it is unpacked and human-readable. Unique items not covered above:

| Files | What they are | Unreal Engine 5 status |
|---|---|---|
| `data/savegame/` | Save game slot format — shows how the original game serialised campaign progress | ✅ Structure referenced to write `MC2SaveGame` and `MC2LogisticsSubsystem` save/load system |
| `art.fst`, `camera.fst` | File System Table — binary manifests listing all packed assets in the shipped game | ➖ Not needed — we use the unpacked source files directly |
| `*.bik` | Bink Video cutscene files | ❌ Not present in repository (see Movies section above) |

---

## Summary of Gaps

| Gap | Severity | What to do |
|---|---|---|
| **ABL mission scripts** — 24 missions need Blueprint Event Graphs built in the Unreal editor | High | Manual work per mission using `MC2MissionScript_Reference.h` as the guide |
| **Niagara particle effects** — 4 effect types (Fireball, Missile Flare, Gauss flare, star) | High | Create Niagara systems in the editor; `MC2FXLibrary.cpp` provides the C++ spawn hooks |
| **Bink Video cutscene videos** — not in repository | Medium | Source from original game disc; convert to MP4; import via Unreal Engine Media Framework |
| **Widget Blueprint visual layouts** — all 20 user interface screens need their layouts built in the Unreal Motion Graphics designer | Medium | Manual work in the Unreal editor per widget |
| **RSP random sound files** — 33 files not parsed by pipeline | Low | Extend `ue_build_sound_cues.py` to read Response files and build exact variant groupings |
| **campaign.fit / tutorial.fit** — campaign progression not a Data Table | Low | Add `DT_Campaign` to `fit_convert.py` output and `ue_import_data_tables.py` |
| **Cameras.fit / Colors.fit** — camera parameters and team colours hardcoded in C++ | Low | Optional: import as Data Tables so designers can adjust values without recompiling |
