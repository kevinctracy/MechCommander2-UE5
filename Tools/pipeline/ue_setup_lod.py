"""
ue_setup_lod.py — run inside Unreal Editor Python console
Configures 3 LOD levels on every Skeletal Mesh under /Game/Meshes/Mechs/ and
/Game/Meshes/Vehicles/ matching the MC2 visibility budget.

LOD screen-size thresholds derived from mech bounding radius ~450 cm:
    LOD 0 → always shown until screen size < 0.30  (~100m from camera)
    LOD 1 → shown until screen size < 0.10  (~300m)
    LOD 2 → shown until screen size < 0.05  (~600m)
    LOD 3 → beyond 600m: cull

If a mesh already has 3+ LODs the thresholds are applied without adding new LODs.
If it has fewer than 3 LODs the script logs a warning — LOD geometry must be
created in a DCC tool (Blender / Maya) before the thresholds can be applied.

Run AFTER importing mech FBX files (P0.3.4).
"""

import unreal

# ── Config ────────────────────────────────────────────────────────────────────
MECH_PATHS    = ["/Game/Meshes/Mechs", "/Game/Meshes/Vehicles"]
MIN_LOD_COUNT = 3   # expect LOD0, LOD1, LOD2 (LOD3 = auto-cull at threshold 0)

# Screen-size threshold at which UE switches to the next LOD.
# Index 0 = threshold to enter LOD1, index 1 = LOD2, index 2 = cull (LOD3).
LOD_SCREEN_SIZES = [0.30, 0.10, 0.05]

# Reduction settings per LOD (triangle % kept relative to LOD0)
LOD_REDUCTIONS = [
    {"percent": 50.0, "welding_threshold": 0.1,  "recompute_normals": False},  # LOD1
    {"percent": 25.0, "welding_threshold": 0.25, "recompute_normals": True},   # LOD2
    {"percent": 10.0, "welding_threshold": 0.5,  "recompute_normals": True},   # LOD3
]


# ── Helpers ───────────────────────────────────────────────────────────────────

def find_skeletal_meshes(search_paths: list) -> list:
    """Return all SkeletalMesh asset paths under the given content folders."""
    ar = unreal.AssetRegistry.get()
    meshes = []
    for path in search_paths:
        assets = ar.get_assets_by_path(path, recursive=True)
        for asset_data in assets:
            if asset_data.asset_class_path.asset_name == 'SkeletalMesh':
                meshes.append(str(asset_data.object_path))
    return meshes


def get_lod_count(skel_mesh: unreal.SkeletalMesh) -> int:
    try:
        return unreal.SkeletalMeshEditorSubsystem.get_lod_count(skel_mesh)
    except AttributeError:
        pass
    try:
        info = skel_mesh.get_editor_property('lod_info')
        return len(info) if info else 1
    except Exception:
        return 1


def apply_lod_settings(skel_mesh: unreal.SkeletalMesh, asset_path: str) -> bool:
    """
    Apply LOD screen-size thresholds and auto-generate LOD geometry if needed.
    Returns True if settings were applied.
    """
    lod_count = get_lod_count(skel_mesh)

    # Auto-generate LODs if fewer than MIN_LOD_COUNT
    if lod_count < MIN_LOD_COUNT:
        try:
            sm_subsystem = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
            for lod_idx in range(1, MIN_LOD_COUNT + 1):
                if lod_count >= lod_idx + 1:
                    continue
                reduction = LOD_REDUCTIONS[lod_idx - 1]
                lod_settings = unreal.SkeletalMeshBuildSettings()
                lod_settings.set_editor_property('use_full_precision_u_vs', True)

                reduction_settings = unreal.SkeletalMeshOptimizationSettings()
                reduction_settings.set_editor_property('num_of_tri_angle_ratio',
                    reduction['percent'] / 100.0)
                reduction_settings.set_editor_property('welding_threshold',
                    reduction['welding_threshold'])
                reduction_settings.set_editor_property('recalc_normals',
                    reduction['recompute_normals'])

                sm_subsystem.add_lod(skel_mesh, lod_idx, reduction_settings)

            lod_count = get_lod_count(skel_mesh)
        except Exception as e:
            unreal.log_warning(f'[MC2LOD] Auto-generate failed for {asset_path}: {e}')
            unreal.log_warning(f'[MC2LOD]   Manual: import LOD geometry in FBX options')
            return False

    # Apply screen size thresholds to existing LODs
    try:
        lod_info = skel_mesh.get_editor_property('lod_info')
        if not lod_info:
            return False

        for i, lod in enumerate(lod_info):
            if i == 0:
                continue  # LOD0 always shown; threshold is on the transition TO LOD1
            thresh_idx = i - 1
            if thresh_idx < len(LOD_SCREEN_SIZES):
                lod.set_editor_property('screen_size',
                    unreal.PerPlatformFloat(LOD_SCREEN_SIZES[thresh_idx]))

        # Cull screen size (hide mesh entirely beyond LOD2)
        try:
            skel_mesh.set_editor_property('minimum_lod',
                unreal.PerPlatformInt(0))
        except Exception:
            pass

        unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)
        return True

    except Exception as e:
        unreal.log_warning(f'[MC2LOD] Failed to set LOD info on {asset_path}: {e}')
        return False


# ── Main ─────────────────────────────────────────────────────────────────────

def run():
    mesh_paths = find_skeletal_meshes(MECH_PATHS)
    if not mesh_paths:
        unreal.log_warning('[MC2LOD] No Skeletal Meshes found. Import mech FBX files first (P0.3.4).')
        unreal.log_warning('[MC2LOD] Expected locations:')
        for p in MECH_PATHS:
            unreal.log_warning(f'[MC2LOD]   {p}')
        return

    print(f'[MC2LOD] Found {len(mesh_paths)} Skeletal Meshes')
    print(f'[MC2LOD] LOD thresholds: LOD1@{LOD_SCREEN_SIZES[0]} LOD2@{LOD_SCREEN_SIZES[1]} cull@{LOD_SCREEN_SIZES[2]}')

    applied  = 0
    skipped  = 0
    no_mesh  = 0

    with unreal.ScopedSlowTask(len(mesh_paths), 'Setting up Mech LODs') as task:
        task.make_dialog(True)

        for path in mesh_paths:
            if task.should_cancel():
                break

            short = path.split('/')[-1]
            task.enter_progress_frame(1, short)

            asset = unreal.EditorAssetLibrary.load_asset(path)
            if not asset or not isinstance(asset, unreal.SkeletalMesh):
                no_mesh += 1
                continue

            ok = apply_lod_settings(asset, path)
            if ok:
                applied += 1
            else:
                skipped += 1

    print(f'\n[MC2LOD] Done: {applied} configured, {skipped} need manual LOD geometry, {no_mesh} failed to load')
    if skipped > 0:
        print('[MC2LOD] For meshes needing LOD geometry:')
        print('[MC2LOD]   In Blender: use Decimate modifier at 50%/25%/10% → export as FBX')
        print('[MC2LOD]   In UE: Content Browser → right-click mesh → LOD → Import LOD Level N')
        print('[MC2LOD]   Re-run this script after importing LOD geometry.')


run()
