"""
run_pipeline.py — MC2 → UE5 master pipeline runner

Orchestrates every import/build step in dependency order.

STAGE 1 (offline, any machine with Python + Blender):
  1. txm_extract.py         — TXM → PNG
  2. extract_all_mission_data.py — FIT/ABL → JSON
  3. ase_to_fbx.py (Blender) — ASE → FBX

STAGE 2 (Unreal Editor Python console — Tools > Execute Python Script):
  Run each script below inside the UE editor in order. They are not
  auto-executed here because UE must own the asset registry.

Usage (offline stages):
    python run_pipeline.py \\
        --source   /path/to/MechCommander2/Source \\
        --fbx-out  /tmp/mc2_fbx \\
        --json-out /tmp/mc2_missions \\
        --png-out  /tmp/mc2_png \\
        --blender  blender            # path to Blender executable
        [--stage {all,extract,fbx}]   # default: all

Then copy /tmp/mc2_fbx and /tmp/mc2_missions to the machine running UE and
execute each Stage 2 script from inside the UE editor console.
"""

import argparse
import subprocess
import sys
import textwrap
from pathlib import Path

HERE = Path(__file__).parent
PIPELINE = HERE / "pipeline"

# ─── helpers ──────────────────────────────────────────────────────────────────

def run(cmd: list[str], label: str) -> None:
    print(f"\n{'='*70}")
    print(f"  {label}")
    print(f"  {' '.join(str(c) for c in cmd)}")
    print(f"{'='*70}")
    result = subprocess.run(cmd, check=False)
    if result.returncode != 0:
        print(f"[ERROR] {label} exited with code {result.returncode}", file=sys.stderr)
        sys.exit(result.returncode)

def print_ue_instructions(fbx_out: Path, json_out: Path, png_out: Path) -> None:
    """Print the ordered list of UE Python console scripts to run after offline stages."""
    scripts = [
        ("ue_import_txm_pngs.py",    f"# PNG_DIR = '{png_out}'"),
        ("ue_import_textures.py",    f"# TGA_SOURCE = '{fbx_out.parent}/Source/Data/Art'"),
        ("ue_import_audio.py",       f"# WAV_SOURCE = '{fbx_out.parent}/Source/Data/Sound'"),
        ("ue_import_data_tables.py", f"# CSV_SOURCE = '{fbx_out.parent}/Tools/pipeline'  (fit_convert.py output)"),
        ("ue_import_meshes.py",      f"# FBX_DIR = '{fbx_out}'"),
        ("ue_import_animations.py",  f"# FBX_DIR = '{fbx_out}'"),
        ("ue_setup_lod.py",          "# (no source path needed — operates on already-imported assets)"),
        ("ue_build_sound_cues.py",   "# (no source path needed — operates on /Game/Audio/)"),
        ("ue_build_sound_mix.py",    "# (no source path needed)"),
        ("ue_build_mission.py",      f"# MISSION_JSON_DIR = '{json_out}'  ;  set MISSION_ID = None for all 24"),
    ]

    print("\n" + "="*70)
    print("  STAGE 2 — Run inside Unreal Editor (Tools > Execute Python Script)")
    print("  Execute scripts in this order:")
    print("="*70)
    for i, (script, note) in enumerate(scripts, 1):
        print(f"\n  {i:2d}. {PIPELINE / script}")
        print(f"      {note}")

    print("\n  After Stage 2 completes, perform these editor tasks:")
    print("   A. Create BP_BattleMech extending AMC2BattleMech — assign SK_BattleMech_Skeleton,")
    print("      Physics Asset, and ABP_BattleMech (see ABP_BattleMech_Reference.h).")
    print("   B. Drag BP_BattleMech into the test level to verify placement (P1.6).")
    print("   C. Profile 20-unit scenario for 60fps target; address any bottlenecks (P9.5).")
    print("   D. Play through all 24 missions end-to-end (P9.8).")
    print()

# ─── stages ───────────────────────────────────────────────────────────────────

def stage_extract_txm(source: Path, png_out: Path) -> None:
    txm_dir = source / "Data" / "TXM"
    if not txm_dir.exists():
        print(f"[SKIP] TXM directory not found: {txm_dir}")
        return
    run([sys.executable, PIPELINE / "txm_extract.py",
         str(txm_dir), str(png_out)],
        "TXM → PNG extraction")

def stage_extract_missions(source: Path, json_out: Path) -> None:
    warriors_dir = source / "Data" / "Missions" / "Warriors"
    run([sys.executable, PIPELINE / "extract_all_mission_data.py",
         "--source", source / "Data",
         "--warriors", warriors_dir,
         "--output", json_out],
        "FIT/ABL → Mission JSON extraction")

def stage_ase_to_fbx(source: Path, fbx_out: Path, blender: str) -> None:
    ase_dir = source / "Data" / "TGL"
    if not ase_dir.exists():
        print(f"[SKIP] ASE directory not found: {ase_dir}")
        return
    run([blender, "--background", "--python", PIPELINE / "ase_to_fbx.py",
         "--", str(ase_dir), str(fbx_out)],
        "ASE → FBX conversion (Blender)")

# ─── main ─────────────────────────────────────────────────────────────────────

def main() -> None:
    ap = argparse.ArgumentParser(
        description="MC2 → UE5 pipeline runner",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=textwrap.dedent("""\
            Examples:
              # Run all offline stages then print UE instructions:
              python run_pipeline.py --source ~/mc2/Source --fbx-out /tmp/mc2_fbx \\
                  --json-out /tmp/mc2_missions --png-out /tmp/mc2_png

              # Only extract textures and missions (skip Blender):
              python run_pipeline.py --source ~/mc2/Source \\
                  --fbx-out /tmp/mc2_fbx --json-out /tmp/mc2_missions \\
                  --png-out /tmp/mc2_png --stage extract
        """),
    )
    ap.add_argument("--source",   required=True, type=Path,
                    help="MC2 source root (contains Data/)")
    ap.add_argument("--fbx-out",  required=True, type=Path,
                    help="Output directory for converted FBX files")
    ap.add_argument("--json-out", required=True, type=Path,
                    help="Output directory for mission JSON files")
    ap.add_argument("--png-out",  required=True, type=Path,
                    help="Output directory for extracted TXM PNGs")
    ap.add_argument("--blender",  default="blender",
                    help="Path to Blender executable (default: 'blender' on PATH)")
    ap.add_argument("--stage", choices=["all", "extract", "fbx"], default="all",
                    help="Which offline stages to run (default: all)")
    args = ap.parse_args()

    args.fbx_out.mkdir(parents=True, exist_ok=True)
    args.json_out.mkdir(parents=True, exist_ok=True)
    args.png_out.mkdir(parents=True, exist_ok=True)

    if args.stage in ("all", "extract"):
        stage_extract_txm(args.source, args.png_out)
        stage_extract_missions(args.source, args.json_out)

    if args.stage in ("all", "fbx"):
        stage_ase_to_fbx(args.source, args.fbx_out, args.blender)

    print_ue_instructions(args.fbx_out, args.json_out, args.png_out)
    print("[DONE] Offline pipeline complete.")

if __name__ == "__main__":
    main()
