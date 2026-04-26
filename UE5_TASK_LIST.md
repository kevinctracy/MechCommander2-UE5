# MC2 → UE5 Task List

Complete these in order. Each stage depends on the previous one.

---

## STAGE 1 — Before Opening UE (Terminal / Blender)

### ✅ 1a. Convert ASE models to FBX
**Done.** 2,927 / 2,947 FBX files produced in `/tmp/mc2_out/fbx/`.
(20 skipped — intentionally empty destroyed-prop files with 0 vertices.)

### ✅ 1b. Extract mission data
**Done.** 24 mission JSON files in `/tmp/mc2_out/missions/`.

### ✅ 1c. Extract TXM textures
**Done.** 100/100 PNGs extracted to `/tmp/mc2_out/png/`.
```
cd /Volumes/projects/personal/MechCommander2-Source
python3 Tools/pipeline/txm_extract.py \
    Source/Data/Textures /tmp/mc2_out/png
```
100 files → PNGs. Takes a few seconds.

---

## STAGE 2 — First-Time Project Setup (UE Editor, once only) ← YOU ARE HERE

### ⬜ 2a. Generate project files
Right-click `UE5Project/MechCommander2/MechCommander2.uproject` → **Generate Project Files**

### ⬜ 2b. Compile
Open the generated solution in your IDE, build config **Development Editor**, hit Build.
Expect first compile to take 5–10 min.

### ⬜ 2c. Enable Python in UE
**Edit → Plugins → search "Python Editor Script Plugin" → Enable → restart editor**

### ⬜ 2d. Create two placeholder maps
Without these UE will warn on every launch:
- **File → New Level → Empty Level** → save as `Content/Maps/MainMenu`
- **File → New Level → Empty Level** → save as `Content/Maps/Loading`

---

## STAGE 3 — Import Assets (UE Editor, Tools → Execute Python Script)

Run each script via **Tools → Execute Python Script** and wait for it to finish
before running the next. Check the Output Log for errors after each one.

> **How to open Output Log:** Window → Output Log

### ⬜ Step 1 — Import TXM PNGs
Edit the path at the top of the script first:
```python
# In ue_import_txm_pngs.py, set:
PNG_DIR = r"/tmp/mc2_out/png"
```
Run: `Tools/pipeline/ue_import_txm_pngs.py`

**What to see:** ~100 new texture assets in `/Game/Textures/TXM/`

---

### ⬜ Step 2 — Import TGA textures
Edit the path at the top:
```python
SOURCE_DIR = r"/Volumes/projects/personal/MechCommander2-Source/Source/Data/Art"
```
Run: `Tools/pipeline/ue_import_textures.py`

**What to see:** Textures sorted into `/Game/Textures/Mechs/`, `/Game/Textures/UI/`, etc.

---

### ⬜ Step 3 — Import audio
Edit the path at the top:
```python
WAV_SOURCE = r"/Volumes/projects/personal/MechCommander2-Source/Source/Data/Sound"
```
Run: `Tools/pipeline/ue_import_audio.py`

**What to see:** ~1,146 sound assets in `/Game/Audio/`, Sound Class hierarchy created.

---

### ⬜ Step 4 — Build sound cues
No path edits needed — operates on `/Game/Audio/` which now exists.

Run: `Tools/pipeline/ue_build_sound_cues.py`

**What to see:** SoundCue assets for multi-variant sounds (weapon fire, etc.)

---

### ⬜ Step 5 — Build sound mix
Run: `Tools/pipeline/ue_build_sound_mix.py`

**What to see:** `SM_MC2Master` Sound Mix asset in `/Game/Audio/`

---

### ⬜ Step 6 — Import data tables (mech/vehicle/pilot stats)
Run: `Tools/pipeline/ue_import_data_tables.py`

**What to see:** 6 DataTable assets in `/Game/Data/`:
- `DT_MechChassis` (34 rows)
- `DT_MechVariants` (42 rows)
- `DT_VehicleTypes` (67 rows)
- `DT_Components` (80 rows)
- `DT_BuildingTypes` (1,088 rows)
- `DT_Pilots` (51 rows)

---

### ⬜ Step 7 — Import meshes
Edit the path at the top:
```python
FBX_DIR = r"/tmp/mc2_out/fbx"
```
Run: `Tools/pipeline/ue_import_meshes.py`

**What to see:** Skeletal meshes in `/Game/Meshes/Mechs/`, `/Game/Meshes/Vehicles/`,
static meshes in `/Game/Meshes/Buildings/`. This takes a few minutes.

---

### ⬜ Step 8 — Import animations
Same FBX_DIR as step 7 — edit if needed.

Run: `Tools/pipeline/ue_import_animations.py`

**What to see:** Animation sequences in `/Game/Animations/Mechs/` — one per
chassis per gesture (Walk, Run, Idle, etc.). Check Output Log for any
"missing ABP-required animation" warnings.

---

### ⬜ Step 9 — Set up LODs
No path edits needed.

Run: `Tools/pipeline/ue_setup_lod.py`

**What to see:** Each skeletal mesh now has 3 LOD levels. You can verify by
opening any mesh asset and clicking the LOD dropdown in the toolbar.

---

### ⬜ Step 10 — Build mission levels
Edit the path at the top:
```python
MISSION_JSON_DIR = r"/tmp/mc2_out/missions"
MISSION_ID = None   # None = build all 24, or set e.g. "mc2_01" for one
```
Run: `Tools/pipeline/ue_build_mission.py`

**What to see:** 24 level assets in `/Game/Maps/Missions/`, each with terrain,
actor placements, trigger volumes, waypoints, and lighting configured.
This is the slow one — budget ~1 min per mission.

---

## STAGE 4 — Blueprint Setup (UE Editor, manual)

These can't be scripted — they require the Blueprint editor UI.

### ⬜ 4a. Create the Physics Asset for mechs
- Open any mech skeletal mesh (e.g. `/Game/Meshes/Mechs/SK_Atlas`)
- **Tools → Create Physics Asset** → accept defaults
- Save as `PA_BattleMech`

### ⬜ 4b. Create `ABP_BattleMech` (Animation Blueprint)
- **Content Browser → Add → Animation → Animation Blueprint**
- Parent class: `AnimInstance`, Skeleton: `SK_BattleMech_Skeleton`
- Save as `/Game/Animations/ABP_BattleMech`
- Build the state machine per `UE5Project/MechCommander2/Source/MechCommander2/Public/Units/ABP_BattleMech_Reference.h`
  (that file is your full implementation guide)

### ⬜ 4c. Create `BP_BattleMech`
- **Content Browser → Add → Blueprint Class → search AMC2BattleMech**
- Save as `/Game/Blueprints/Units/BP_BattleMech`
- In the Blueprint's Components panel:
  - Assign a mech Skeletal Mesh to the SkeletalMeshComponent
  - Set **Anim Class** to `ABP_BattleMech`
  - Set **Physics Asset** to `PA_BattleMech`
- Compile & Save

### ⬜ 4d. Smoke-test
- **File → New Level → Empty Level** → save as `Content/Maps/TestLevel`
- Drag `BP_BattleMech` from the content browser into the viewport
- Hit **Play** — the mech should appear. It won't move yet (no orders issued),
  but it should be visible with physics and animations ready.

---

## STAGE 5 — Wire Up Game Settings (UE Editor)

### ⬜ 5a. Set default game mode
**Edit → Project Settings → Maps & Modes:**
- Default GameMode: `MC2GameMode`
- Default Map: `MainMenu`

### ⬜ 5b. Set sound mix
**Edit → Project Settings → Audio:**
- Default Base Sound Mix: `SM_MC2Master`

### ⬜ 5c. Set input config
**Edit → Project Settings → Input:**
- Default Input Config: select the `MC2InputConfig` asset

---

## Done

At this point you have a compilable project with all assets imported, all 24
mission levels built, and a working mech you can drop into any level.

**What still needs manual work (Blueprint visual layouts):**
Every `WBP_*` widget Blueprint needs its visual layout built in the UMG designer.
The C++ backends are all wired up — you just need to place the buttons, text
blocks, and images that the C++ code drives. See the header file for each widget
for the exact list of what each BlueprintImplementableEvent expects.
