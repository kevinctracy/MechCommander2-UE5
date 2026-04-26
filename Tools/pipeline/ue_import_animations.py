"""
ue_import_animations.py — run inside Unreal Editor Python console
Imports mech FBX animation files (produced by ase_to_fbx.py) into UE Animation
Sequences, grouped by the 25 gesture types from mech3d.h.

Prerequisites:
    1. Run ase_to_fbx.py via Blender to produce FBX files:
         blender --background --python ase_to_fbx.py -- \\
             --input  Source/Data/TGL/ \\
             --output /tmp/mc2_fbx/
    2. Set FBX_DIR and SKELETON_PATH below.
    3. Open the UE project with the target skeleton loaded.
    4. Run via Tools > Execute Python Script.

Naming convention expected from ase_to_fbx.py output:
    {ChassisName}_{GestureName}.fbx
    e.g. Atlas_Walk.fbx, Atlas_Run.fbx, Atlas_Jump.fbx

All animations for a chassis share the same skeleton (SK_BattleMech_Skeleton).
"""

import unreal
from pathlib import Path

# ── Config ────────────────────────────────────────────────────────────────────
FBX_DIR       = r"/tmp/mc2_fbx/animations"
ANIM_PKG      = "/Game/Animations/Mechs"
SKELETON_PATH = "/Game/Meshes/Mechs/SK_BattleMech_Skeleton"

# ── Gesture ID → canonical UE animation name ──────────────────────────────────
# Maps mech3d.h gesture constants to the ABP_BattleMech_Reference animation names.
# The FBX suffix (after chassis name) is the key; value is the UE asset suffix.
# Obsolete gestures (StandToWalk, WalkToRun etc.) import but are not used in ABP.
GESTURE_MAP = {
    # FBX filename suffix (lowercase) → UE asset suffix
    "park":           "Park",
    "standtopark":    "StandToPark",
    "stand":          "Stand",
    "standtowalk":    "StandToWalk",       # obsolete
    "walk":           "Walk",
    "parktostand":    "ParkToStand",
    "walktorun":      "WalkToRun",         # obsolete
    "run":            "Run",
    "runtowalk":      "RunToWalk",         # obsolete; also AtlasRNtoWK
    "rntowk":         "RunToWalk",
    "reverse":        "Reverse",
    "standtoreverse": "StandToReverse",    # obsolete
    "sttorev":        "StandToReverse",
    "limpLeft":       "LimpLeft",
    "limpleft":       "LimpLeft",
    "limpright":      "LimpRight",
    "idle":           "Idle",
    "fallbackward":   "FallBackward",
    "fallbackwarddam": "FallBackward",
    "fallforward":    "FallForward",
    "fallforwarddam": "FallForward",
    "hitfront":       "HitFront",
    "hitback":        "HitBack",
    "hitleft":        "HitLeft",
    "hitright":       "HitRight",
    "jump":           "Jump",
    "rollover":       "Rollover",
    "getupback":      "GetUpBack",
    "getupfront":     "GetUpFront",
    "fallenforward":  "FallenForward",
    "fallenbackward": "FallenBackward",
    # Arm animations (for hardpoint socket offsets — not used in main ABP)
    "leftarm":        "LeftArm",
    "rightarm":       "RightArm",
    # L1 variants = LOD1 reduced-poly animations
    "l1":             None,                # skip LOD variants
}

# ABP uses these gesture → UE asset name mappings (for reference/annotation)
ABP_REQUIRED = {
    "Idle", "Walk", "Run", "LimpLeft", "LimpRight",
    "Jump", "FallForward", "FallBackward", "FallenForward", "FallenBackward",
    "GetUpFront", "GetUpBack",
}


def parse_chassis_gesture(stem: str) -> tuple[str, str] | None:
    """
    Parse 'AtlasWalk' or 'Atlas_Walk' → ('Atlas', 'walk').
    Returns None if the stem doesn't match a known chassis.
    """
    # Try underscore split first
    parts = stem.split('_', 1)
    if len(parts) == 2:
        return parts[0], parts[1].lower()

    # Try CamelCase split: find where the gesture keyword starts
    lower = stem.lower()
    for suffix in sorted(GESTURE_MAP.keys(), key=len, reverse=True):
        if lower.endswith(suffix):
            chassis = stem[: len(stem) - len(suffix)]
            return chassis, suffix

    return None


def import_animation(fbx_path: str, chassis: str, gesture_suffix: str,
                     skeleton) -> bool:
    """Import a single FBX file as an Animation Sequence asset."""
    ue_suffix = GESTURE_MAP.get(gesture_suffix.lower())
    if ue_suffix is None:
        return False  # skip (LOD variant or unknown)

    dest_pkg  = f"{ANIM_PKG}/{chassis}"
    asset_name = f"A_{chassis}_{ue_suffix}"
    full_pkg   = f"{dest_pkg}/{asset_name}"

    if unreal.EditorAssetLibrary.does_asset_exist(full_pkg):
        return True  # already imported

    task = unreal.AssetImportTask()
    task.filename         = fbx_path
    task.destination_path = dest_pkg
    task.destination_name = asset_name
    task.automated        = True
    task.replace_existing = True
    task.save             = False

    opts = unreal.FbxImportUI()
    opts.import_mesh             = False
    opts.import_textures         = False
    opts.import_materials        = False
    opts.import_as_skeletal      = False
    opts.import_animations       = True
    opts.create_physics_asset    = False
    opts.mesh_type_to_import     = unreal.FBXImportType.FBXIT_ANIMATION
    opts.skeleton                = skeleton

    anim_opts = opts.anim_sequence_import_data
    anim_opts.import_uniform_scale      = 1.0
    anim_opts.use_default_sample_rate   = True
    anim_opts.set_editor_property('animation_length',
        unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)

    task.options = opts
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    return unreal.EditorAssetLibrary.does_asset_exist(full_pkg)


def run():
    fbx_dir = Path(FBX_DIR)
    if not fbx_dir.exists():
        unreal.log_error(f'[MC2Anim] FBX directory not found: {FBX_DIR}')
        unreal.log_error('[MC2Anim] Run ase_to_fbx.py first:')
        unreal.log_error('[MC2Anim]   blender --background --python ase_to_fbx.py -- \\')
        unreal.log_error(f'[MC2Anim]     --input Source/Data/TGL/ --output {FBX_DIR}')
        return

    # Load skeleton
    skeleton = unreal.EditorAssetLibrary.load_asset(SKELETON_PATH)
    if not skeleton:
        unreal.log_error(f'[MC2Anim] Skeleton not found: {SKELETON_PATH}')
        unreal.log_error('[MC2Anim] Import the base mech FBX first (P0.3.4), then rerun.')
        return

    fbx_files = sorted(fbx_dir.glob('*.fbx'))
    if not fbx_files:
        unreal.log_warning(f'[MC2Anim] No FBX files found in {FBX_DIR}')
        return

    print(f'[MC2Anim] Found {len(fbx_files)} FBX animation files')

    imported    = 0
    skipped     = 0
    failed      = 0
    abp_covered: set[str] = set()

    by_chassis: dict[str, list] = {}
    for f in fbx_files:
        parsed = parse_chassis_gesture(f.stem)
        if parsed:
            chassis, gesture = parsed
            by_chassis.setdefault(chassis, []).append((f, gesture))
        else:
            skipped += 1

    print(f'[MC2Anim] {len(by_chassis)} chassis found')

    with unreal.ScopedSlowTask(len(fbx_files), 'Importing Mech Animations') as task:
        task.make_dialog(True)

        for chassis, entries in sorted(by_chassis.items()):
            for fbx_path, gesture in entries:
                if task.should_cancel():
                    break
                task.enter_progress_frame(1, f'{chassis}_{gesture}')

                ok = import_animation(str(fbx_path), chassis, gesture, skeleton)
                if ok:
                    imported += 1
                    ue_suffix = GESTURE_MAP.get(gesture.lower(), '')
                    if ue_suffix in ABP_REQUIRED:
                        abp_covered.add(ue_suffix)
                else:
                    if GESTURE_MAP.get(gesture.lower()) is None:
                        skipped += 1
                    else:
                        failed += 1

    print(f'\n[MC2Anim] Done: {imported} imported, {skipped} skipped (LOD/obsolete), {failed} failed')

    missing_abp = ABP_REQUIRED - abp_covered
    if missing_abp:
        print(f'[MC2Anim] WARNING: ABP_BattleMech requires these missing animations:')
        for name in sorted(missing_abp):
            print(f'[MC2Anim]   A_{{Chassis}}_{name}')
    else:
        print('[MC2Anim] All 12 ABP_BattleMech required animations are present.')

    print(f'[MC2Anim] Animation assets in: {ANIM_PKG}/')
    print('[MC2Anim] Gesture → ABP asset mapping:')
    print('[MC2Anim]   GestureIdle(13)        → A_*_Idle')
    print('[MC2Anim]   GestureWalk(4)          → A_*_Walk  (→ BS_MechWalk blend space)')
    print('[MC2Anim]   GestureRun(7)           → A_*_Run')
    print('[MC2Anim]   GestureLimpLeft/Right   → A_*_LimpLeft / A_*_LimpRight')
    print('[MC2Anim]   GestureJump(20)         → A_*_Jump')
    print('[MC2Anim]   GestureFallForward(15)  → A_*_FallForward')
    print('[MC2Anim]   GestureFallBackward(14) → A_*_FallBackward')
    print('[MC2Anim]   GestureGetUp(22)        → A_*_GetUpFront + A_*_GetUpBack')
    print('[MC2Anim]   HitFront/Back/L/R       → A_*_Hit* (not in ABP; spawn Niagara FX instead)')


run()
