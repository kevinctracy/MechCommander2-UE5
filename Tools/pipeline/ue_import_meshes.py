"""
ue_import_meshes.py — run inside Unreal Editor Python console
Imports mech, vehicle, building, and prop FBX files produced by ase_to_fbx.py
into UE Skeletal Mesh and Static Mesh assets.

Prerequisites:
    1. Run ase_to_fbx.py via Blender:
         blender --background --python ase_to_fbx.py -- \\
             Source/Data/TGL/ /tmp/mc2_fbx/
    2. Set FBX_ROOT below to the ase_to_fbx.py output directory.
    3. Run via Tools > Execute Python Script with the UE project open.

Expected directory structure from ase_to_fbx.py:
    /tmp/mc2_fbx/
        mechs/<ChassisName>/<ChassisName>.fbx        ← base Skeletal Mesh
        mechs/<ChassisName>/<ChassisName>_L1.fbx     ← LOD1
        mechs/<ChassisName>/<ChassisName>_Dest.fbx   ← destroyed Static Mesh
        vehicles/<VehicleName>/<VehicleName>.fbx
        buildings/<BuildingName>/<BuildingName>.fbx   ← Static Mesh
        props/<PropName>/<PropName>.fbx               ← Static Mesh

What this script does:
    - mechs/ and vehicles/ → SkeletalMesh with shared SK_BattleMech_Skeleton
    - buildings/ and props/ → StaticMesh
    - *_Dest.fbx files → StaticMesh (destroyed variant)
    - *_L1.fbx files → imported as LOD1 on the base Skeletal Mesh
"""

import unreal
from pathlib import Path

# ── Config ────────────────────────────────────────────────────────────────────
FBX_ROOT = r"/tmp/mc2_fbx"

SKELETON_PATH = "/Game/Meshes/Mechs/SK_BattleMech_Skeleton"
VEHICLE_SKEL  = "/Game/Meshes/Vehicles/SK_Vehicle_Skeleton"

MESH_PACKAGES = {
    "mechs":     "/Game/Meshes/Mechs",
    "vehicles":  "/Game/Meshes/Vehicles",
    "buildings": "/Game/Meshes/Buildings",
    "props":     "/Game/Meshes/Props",
}


# ── Helpers ───────────────────────────────────────────────────────────────────

def is_skeletal(category: str) -> bool:
    return category in ("mechs", "vehicles")


def make_import_task(fbx_path: str, dest_pkg: str, dest_name: str,
                     skeletal: bool, skeleton) -> unreal.AssetImportTask:
    task = unreal.AssetImportTask()
    task.filename         = fbx_path
    task.destination_path = dest_pkg
    task.destination_name = dest_name
    task.automated        = True
    task.replace_existing = True
    task.save             = False

    opts = unreal.FbxImportUI()
    opts.import_textures        = False
    opts.import_materials       = False
    opts.import_animations      = False
    opts.create_physics_asset   = skeletal

    if skeletal:
        opts.import_mesh        = True
        opts.import_as_skeletal = True
        opts.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
        opts.skeleton            = skeleton
        skel_opts = opts.skeletal_mesh_import_data
        skel_opts.import_morph_targets      = False
        skel_opts.convert_scene             = True
        skel_opts.import_uniform_scale      = 1.0
        skel_opts.normal_import_method      = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    else:
        opts.import_mesh        = True
        opts.import_as_skeletal = False
        opts.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
        stat_opts = opts.static_mesh_import_data
        stat_opts.combine_meshes         = True
        stat_opts.generate_lightmap_u_vs = True
        stat_opts.convert_scene          = True
        stat_opts.import_uniform_scale   = 1.0

    task.options = opts
    return task


def import_lod(base_mesh, lod_fbx: str, lod_index: int) -> bool:
    """Import an FBX as LOD N on an existing Skeletal Mesh."""
    try:
        sm_sub = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
        result = sm_sub.import_lod(base_mesh, lod_index, lod_fbx)
        return result != unreal.LODImportResult.LODIMPORT_ERROR
    except Exception as e:
        unreal.log_warning(f'[MC2Mesh] LOD import failed: {e}')
        return False


# ── Main ─────────────────────────────────────────────────────────────────────

def run():
    fbx_root = Path(FBX_ROOT)
    if not fbx_root.exists():
        unreal.log_error(f'[MC2Mesh] FBX root not found: {FBX_ROOT}')
        unreal.log_error('[MC2Mesh] Run ase_to_fbx.py first:')
        unreal.log_error('[MC2Mesh]   blender --background --python ase_to_fbx.py -- \\')
        unreal.log_error(f'[MC2Mesh]     Source/Data/TGL/ {FBX_ROOT}')
        return

    # Load shared skeletons
    mech_skeleton    = unreal.EditorAssetLibrary.load_asset(SKELETON_PATH)
    vehicle_skeleton = unreal.EditorAssetLibrary.load_asset(VEHICLE_SKEL)
    if not mech_skeleton:
        unreal.log_warning(f'[MC2Mesh] Mech skeleton not found at {SKELETON_PATH}')
        unreal.log_warning('[MC2Mesh] Will create new skeleton on first mech import.')

    # Gather all base FBX files (exclude animations — those go to ue_import_animations.py)
    ANIM_SUFFIXES = {
        "walk", "run", "idle", "fall", "jump", "limp", "stand", "park",
        "reverse", "hit", "roll", "getup", "fallen", "walktorun", "rntowk",
        "wktorn", "wktost", "sttowk", "sttorev", "parktostand", "standtopark",
        "leftarm", "rightarm",
    }

    import_tasks   = []
    lod_queue      = []  # (base_pkg, lod_fbx_path, lod_index)
    dest_queue     = []  # (base_pkg + "_Destroyed", dest_fbx)

    total_base = 0

    for category, pkg_root in MESH_PACKAGES.items():
        cat_dir = fbx_root / category
        if not cat_dir.exists():
            continue

        skeletal = is_skeletal(category)
        skeleton = mech_skeleton if category == "mechs" else (
                   vehicle_skeleton if category == "vehicles" else None)

        unreal.EditorAssetLibrary.make_directory(pkg_root)

        for chassis_dir in sorted(cat_dir.iterdir()):
            if not chassis_dir.is_dir():
                continue
            chassis = chassis_dir.name

            for fbx in sorted(chassis_dir.glob("*.fbx")):
                stem_lower = fbx.stem.lower()
                chassis_lower = chassis.lower()

                # Determine what this FBX is
                rel = stem_lower.replace(chassis_lower, "").strip("_")

                if rel in ("", chassis_lower):
                    # Base mesh
                    dest_pkg  = f"{pkg_root}/{chassis}"
                    dest_name = f"SK_{chassis}" if skeletal else f"SM_{chassis}"
                    task = make_import_task(str(fbx), dest_pkg, dest_name,
                                            skeletal, skeleton)
                    import_tasks.append(task)
                    total_base += 1

                elif rel in ("l1", "l2", "l3"):
                    lod_idx = int(rel[1])
                    base_pkg = f"{pkg_root}/{chassis}/SK_{chassis}"
                    lod_queue.append((base_pkg, str(fbx), lod_idx))

                elif rel in ("dest", "destroyed", "x"):
                    dest_dest_pkg  = f"{pkg_root}/{chassis}"
                    dest_dest_name = f"SM_{chassis}_Destroyed"
                    task = make_import_task(str(fbx), dest_dest_pkg, dest_dest_name,
                                            False, None)
                    import_tasks.append(task)

                else:
                    # Check if it's an animation (skip — handled by ue_import_animations.py)
                    if any(rel.endswith(s) or rel == s for s in ANIM_SUFFIXES):
                        continue
                    # Unknown suffix — import as prop
                    dest_pkg  = f"{pkg_root}/{chassis}"
                    dest_name = f"SM_{chassis}_{rel}"
                    task = make_import_task(str(fbx), dest_pkg, dest_name,
                                            False, None)
                    import_tasks.append(task)

    print(f'[MC2Mesh] Importing {len(import_tasks)} mesh FBX files ({total_base} base meshes)')

    with unreal.ScopedSlowTask(len(import_tasks) + len(lod_queue),
                               'Importing MC2 Meshes') as prog:
        prog.make_dialog(True)

        # Batch import base meshes in groups of 20
        BATCH = 20
        for i in range(0, len(import_tasks), BATCH):
            if prog.should_cancel():
                break
            batch = import_tasks[i:i + BATCH]
            for t in batch:
                prog.enter_progress_frame(1, Path(t.filename).stem)
            unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(batch)

        # Save all imported assets
        unreal.EditorAssetLibrary.save_directory('/Game/Meshes', recursive=True,
                                                  only_if_is_dirty=False)

        # Import LOD levels onto existing base meshes
        for base_pkg, lod_fbx, lod_idx in lod_queue:
            if prog.should_cancel():
                break
            prog.enter_progress_frame(1, f'LOD{lod_idx}: {Path(lod_fbx).stem}')
            base_asset = unreal.EditorAssetLibrary.load_asset(base_pkg)
            if base_asset and isinstance(base_asset, unreal.SkeletalMesh):
                import_lod(base_asset, lod_fbx, lod_idx)

    # Final save
    unreal.EditorAssetLibrary.save_directory('/Game/Meshes', recursive=True,
                                              only_if_is_dirty=False)

    print(f'\n[MC2Mesh] Done. Run ue_setup_lod.py next to configure LOD screen-size thresholds.')
    print('[MC2Mesh] Then run ue_import_animations.py to import animation sequences.')
    print('[MC2Mesh] Assets in:')
    for cat, pkg in MESH_PACKAGES.items():
        print(f'[MC2Mesh]   {cat:10s} → {pkg}/')


run()
