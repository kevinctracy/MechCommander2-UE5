"""
MC2 ASE -> FBX Batch Converter (Blender Python Script)

Run this INSIDE Blender via: blender --background --python ase_to_fbx.py -- <input_dir> <output_dir>

Converts all 2,947 ASE files from Source/Data/TGL/ to FBX for UE5 import.

Strategy:
  - Mechs have multiple ASE files per chassis:
      Atlas.ase              (base mesh / torso)
      AtlasRun.ase           (animation)
      AtlasIdle.ase          (animation)
      AtlasWKtoRN.ase        (walk-to-run transition)
      AtlasLimpLeft.ase      (limp animation)
      AtlasFallForward.ase   (fall animation)
      AtlasGetUpFront.ase    (get-up animation)
      AtlasJump.ase          (jump)
      AtlasHitFront.ase      (hit reaction)
      AtlasLeftArm.ase       (detachable arm prop)
      AtlasL1.ase            (LOD 1)
      AtlasX.ase             (destroyed wreck)
  - Vehicles/buildings: single or few ASE files

Output groups:
  fbx/mechs/<ChassisName>/                     <- base mesh + all animations
  fbx/vehicles/<VehicleName>/
  fbx/buildings/<BuildingName>/
  fbx/props/<PropName>/

UE5 import notes:
  - Import each chassis' base .fbx as Skeletal Mesh
  - Import animation .fbx files as Animation Sequences, targeting the chassis skeleton
  - Import buildings/props/vehicles as Static Meshes
  - Set FBX import scale to 0.01 (ASE units are cm, UE is cm but ASE default export may differ)
  - Enable "Import Textures" = False (we handle textures separately)

Usage from shell:
  blender --background --python ase_to_fbx.py -- Source/Data/TGL/ fbx_output/

Alternatively, use the standalone ase_to_fbx_standalone.py which uses the
open-source pyassimp library without requiring Blender.
"""

import sys
import os
import re
from pathlib import Path

# When running inside Blender
try:
    import bpy
    IN_BLENDER = True
except ImportError:
    IN_BLENDER = False


# ---------------------------------------------------------------------------
# ASE file grouping logic (works standalone for planning / manifest generation)
# ---------------------------------------------------------------------------

# Animation suffix patterns (case-insensitive)
ANIM_SUFFIXES = [
    "run", "walk", "idle", "park", "stand",
    "fallforward", "fallbackward", "fallback",
    "getupfront", "getupback", "getup",
    "limpright", "limpleft", "limp",
    "jump",
    "hitfront", "hitrear", "hitback", "hitleft", "hitright",
    "parktostand", "standtopark",
    "wktorn", "rntowk", "sttowk", "wktost",
    "rntosk",  # run to strafe
]

# LOD / variant suffixes
LOD_PATTERNS = [r"l\d+$", r"dam(l\d+)?$", r"x$", r"enc$", r"prop$"]


def classify_ase(stem: str) -> tuple:
    """
    Returns (base_name, kind) where kind is one of:
      'base', 'animation', 'lod', 'destroyed', 'arm', 'prop'
    """
    s = stem.lower()

    # Check for animation suffix
    for suffix in ANIM_SUFFIXES:
        if s.endswith(suffix):
            base = stem[:len(stem)-len(suffix)]
            return base.rstrip("_"), "animation"

    # LOD
    for pat in LOD_PATTERNS:
        m = re.search(pat, s)
        if m:
            base = stem[:m.start()]
            kind = "destroyed" if "dam" in s or s.endswith("x") else "lod"
            return base.rstrip("_"), kind

    # Detachable arm
    if s.endswith("arm") or "arm" in s[-6:]:
        base = re.sub(r"(left|right)?arm$", "", s, flags=re.I)
        return base.rstrip("_"), "arm"

    return stem, "base"


def group_ase_files(tgl_dir: Path) -> dict:
    """
    Group all ASE files by chassis/object name.
    Returns { chassis_name: { 'base': [...], 'animation': [...], 'lod': [...], ... } }
    """
    groups: dict = {}
    for ase in sorted(tgl_dir.glob("**/*.ase")):
        stem = ase.stem
        base_name, kind = classify_ase(stem)
        base_key = base_name.lower()
        if base_key not in groups:
            groups[base_key] = {"display_name": base_name, "base": [], "animation": [], "lod": [], "destroyed": [], "arm": [], "prop": []}
        groups[base_key].setdefault(kind, []).append(ase)
    return groups


# ---------------------------------------------------------------------------
# Blender conversion path
# ---------------------------------------------------------------------------

def convert_with_blender(tgl_dir: Path, out_dir: Path):
    """Main conversion loop using Blender's Python API."""
    groups = group_ase_files(tgl_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    ok = 0
    fail = 0

    # Determine object category from INI file presence / name heuristics
    mech_names = _load_mech_names(tgl_dir.parent / "Objects")

    for key, group in groups.items():
        display = group["display_name"]
        all_files = group["base"] + group["animation"] + group["lod"] + group["arm"]

        if key in mech_names or any(key.startswith(mn) for mn in mech_names):
            sub = out_dir / "mechs" / display
        elif _is_vehicle(tgl_dir, key):
            sub = out_dir / "vehicles" / display
        else:
            sub = out_dir / "buildings" / display

        sub.mkdir(parents=True, exist_ok=True)

        for ase_path in all_files:
            try:
                _import_export_ase(ase_path, sub / (ase_path.stem + ".fbx"))
                ok += 1
            except Exception as e:
                print(f"  FAIL {ase_path.name}: {e}")
                fail += 1

    print(f"\nConverted: {ok} ok, {fail} failed")


def _import_export_ase(src: Path, dst: Path):
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()

    bpy.ops.import_scene.ase(filepath=str(src))

    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.export_scene.fbx(
        filepath=str(dst),
        use_selection=True,
        global_scale=1.0,
        apply_scale_options="FBX_SCALE_NONE",
        axis_forward="-Z",
        axis_up="Y",
        bake_space_transform=True,
        use_mesh_modifiers=True,
        mesh_smooth_type="FACE",
        use_armature_deform_only=False,
        add_leaf_bones=False,
        embed_textures=False,
    )


def _load_mech_names(objects_dir: Path) -> set:
    """Read mech chassis names from per-mech CSV files (atlas, madcat, etc.)"""
    known = set()
    if not objects_dir.exists():
        return known
    skip = {"compbas", "variants", "pilots", "badpilots", "hbstatz", "effects", "heat"}
    for csv_path in objects_dir.glob("*.csv"):
        if csv_path.stem.lower() not in skip:
            known.add(csv_path.stem.lower())
    return known


def _is_vehicle(tgl_dir: Path, key: str) -> bool:
    ini = tgl_dir / f"{key}.ini"
    if ini.exists():
        content = ini.read_text(errors="replace").lower()
        return "vehicle" in content or "apc" in content or "helicopter" in content
    return False


# ---------------------------------------------------------------------------
# Standalone manifest generator (no Blender required)
# ---------------------------------------------------------------------------

def generate_manifest(tgl_dir: Path, out_path: Path):
    """
    Without Blender, generate a conversion manifest CSV that documents
    every ASE file, its group, and its intended output FBX path.
    Use this to drive a Blender batch job or a pyassimp-based converter.
    """
    groups = group_ase_files(tgl_dir)
    rows = []
    for key, group in groups.items():
        display = group["display_name"]
        for kind in ("base", "animation", "lod", "destroyed", "arm", "prop"):
            for ase in group.get(kind, []):
                rows.append({
                    "ASEFile":    str(ase.relative_to(tgl_dir)),
                    "Group":      display,
                    "Kind":       kind,
                    "TargetFBX":  f"{display}/{ase.stem}.fbx",
                })

    import csv as csv_mod
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_path, "w", newline="") as f:
        writer = csv_mod.DictWriter(f, fieldnames=["ASEFile", "Group", "Kind", "TargetFBX"])
        writer.writeheader()
        writer.writerows(rows)
    print(f"Manifest written: {out_path} ({len(rows)} entries, {len(groups)} groups)")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    # Strip blender's own args before "--"
    if "--" in sys.argv:
        args = sys.argv[sys.argv.index("--") + 1:]
    else:
        args = sys.argv[1:]

    if len(args) < 2:
        print("Usage: blender --background --python ase_to_fbx.py -- <tgl_dir> <out_dir>")
        print("   or: python ase_to_fbx.py <tgl_dir> <manifest.csv>   (manifest-only mode)")
        sys.exit(1)

    tgl_dir = Path(args[0])
    out_path = Path(args[1])

    if IN_BLENDER:
        convert_with_blender(tgl_dir, out_path)
    else:
        # Running plain Python: generate manifest only
        manifest = out_path if out_path.suffix == ".csv" else out_path / "ase_manifest.csv"
        generate_manifest(tgl_dir, manifest)


if __name__ == "__main__":
    main()
