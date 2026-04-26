"""
MC2 ASE -> FBX Batch Converter (Blender Python Script)

Run this INSIDE Blender via: blender --background --python ase_to_fbx.py -- <input_dir> <output_dir>

Does NOT require any Blender add-ons — parses ASE files directly using the
Blender Python API (bpy.data) and exports FBX using Blender's built-in exporter.

Usage from shell:
  blender --background --python ase_to_fbx.py -- Source/Data/TGL/ /tmp/mc2_out/fbx/

Running without Blender (manifest mode):
  python3 ase_to_fbx.py Source/Data/TGL/ manifest.csv
"""

import sys
import re
from pathlib import Path
from typing import Optional

try:
    import bpy
    import bmesh
    from mathutils import Vector, Matrix
    IN_BLENDER = True
except ImportError:
    IN_BLENDER = False


# ── ASE classification ────────────────────────────────────────────────────────

ANIM_SUFFIXES = [
    "fallbackwarddam", "fallforwarddam",
    "fallbackward", "fallforward", "fallback",
    "getupfront", "getupback", "getup",
    "limpright", "limpleft", "limp",
    "parktostand", "standtopark",
    "wktorn", "rntowk", "sttowk", "wktost", "rntosk",
    "hitfront", "hitrear", "hitleft", "hitright", "hitback",
    "run", "walk", "idle", "park", "stand", "jump",
]

LOD_RE = re.compile(r"(l\d+|dam\w*|enc|prop|x)$", re.IGNORECASE)


def classify_ase(stem: str) -> tuple[str, str]:
    s = stem.lower()
    for suffix in ANIM_SUFFIXES:
        if s.endswith(suffix):
            base = stem[:len(stem) - len(suffix)].rstrip("_")
            return base, "animation"
    m = LOD_RE.search(s)
    if m:
        base = stem[:m.start()].rstrip("_")
        kind = "destroyed" if ("dam" in m.group() or s.endswith("x")) else "lod"
        return base, kind
    if re.search(r"(left|right)?arm$", s):
        base = re.sub(r"(left|right)?arm$", "", s)
        return base.rstrip("_"), "arm"
    return stem, "base"


def group_ase_files(tgl_dir: Path) -> dict:
    groups: dict = {}
    for ase in sorted(tgl_dir.glob("**/*.ase")):
        base_name, kind = classify_ase(ase.stem)
        key = base_name.lower()
        if key not in groups:
            groups[key] = {"display_name": base_name,
                           "base": [], "animation": [], "lod": [],
                           "destroyed": [], "arm": [], "prop": []}
        groups[key].setdefault(kind, []).append(ase)
    return groups


def _load_mech_names(objects_dir: Path) -> set:
    skip = {"compbas", "variants", "pilots", "badpilots", "hbstatz", "effects", "heat"}
    known = set()
    if objects_dir.exists():
        for p in objects_dir.glob("*.csv"):
            if p.stem.lower() not in skip:
                known.add(p.stem.lower())
    return known


# ── ASE parser ────────────────────────────────────────────────────────────────

def _tok(line: str) -> list[str]:
    return line.split()


class ASEMesh:
    def __init__(self, name: str):
        self.name = name
        self.verts: list[tuple] = []
        self.faces: list[tuple] = []
        self.uvs: list[tuple] = []
        self.uv_faces: list[tuple] = []
        self.transform: list[list] = [[1,0,0,0],[0,1,0,0],[0,0,1,0],[0,0,0,1]]


def parse_ase(path: Path) -> list[ASEMesh]:
    """Parse an ASE file and return a list of mesh objects."""
    try:
        text = path.read_text(encoding="latin-1", errors="replace")
    except Exception as e:
        raise RuntimeError(f"Cannot read {path}: {e}")

    lines = text.splitlines()
    meshes: list[ASEMesh] = []
    i = 0
    n = len(lines)

    def peek() -> str:
        return lines[i].strip() if i < n else ""

    while i < n:
        line = lines[i].strip()
        i += 1

        if line.startswith("*GEOMOBJECT"):
            mesh = ASEMesh("object")
            depth = 1  # opening { is on the *GEOMOBJECT line already consumed

            while i < n:
                ln = lines[i].strip()
                i += 1

                if "{" in ln:
                    depth += ln.count("{")
                if "}" in ln:
                    depth -= ln.count("}")

                t = ln.split()
                if not t:
                    continue

                key = t[0]

                if key == "*NODE_NAME" and len(t) > 1:
                    mesh.name = t[1].strip('"')

                elif key == "*TM_ROW0" and len(t) >= 4:
                    mesh.transform[0] = [float(t[1]), float(t[2]), float(t[3]), 0]
                elif key == "*TM_ROW1" and len(t) >= 4:
                    mesh.transform[1] = [float(t[1]), float(t[2]), float(t[3]), 0]
                elif key == "*TM_ROW2" and len(t) >= 4:
                    mesh.transform[2] = [float(t[1]), float(t[2]), float(t[3]), 0]
                elif key == "*TM_ROW3" and len(t) >= 4:
                    mesh.transform[3] = [float(t[1]), float(t[2]), float(t[3]), 1]

                elif key == "*MESH_VERTEX" and len(t) >= 5:
                    mesh.verts.append((float(t[2]), float(t[3]), float(t[4])))

                elif key == "*MESH_FACE" and len(t) >= 7:
                    # *MESH_FACE N: A: v1 B: v2 C: v3 ...
                    try:
                        a = int(t[3])
                        b = int(t[5])
                        c = int(t[7])
                        mesh.faces.append((a, b, c))
                    except (ValueError, IndexError):
                        pass

                elif key == "*MESH_TVERT" and len(t) >= 4:
                    mesh.uvs.append((float(t[2]), float(t[3])))

                elif key == "*MESH_TFACE" and len(t) >= 5:
                    try:
                        mesh.uv_faces.append((int(t[2]), int(t[3]), int(t[4])))
                    except ValueError:
                        pass

                if depth <= 0:
                    break

            if mesh.verts and mesh.faces:
                meshes.append(mesh)

    return meshes


# ── Blender mesh creation & FBX export ───────────────────────────────────────

def _build_blender_mesh(ase_mesh: ASEMesh) -> Optional["bpy.types.Object"]:
    me = bpy.data.meshes.new(ase_mesh.name)
    me.from_pydata(ase_mesh.verts, [], ase_mesh.faces)
    me.update()

    # Apply UV data if present and counts match
    if ase_mesh.uvs and ase_mesh.uv_faces and len(ase_mesh.uv_faces) == len(ase_mesh.faces):
        uv_layer = me.uv_layers.new(name="UVMap")
        for fi, (ai, bi, ci) in enumerate(ase_mesh.uv_faces):
            try:
                loop_start = me.polygons[fi].loop_start
                uv_layer.data[loop_start].uv = ase_mesh.uvs[ai]
                uv_layer.data[loop_start + 1].uv = ase_mesh.uvs[bi]
                uv_layer.data[loop_start + 2].uv = ase_mesh.uvs[ci]
            except IndexError:
                pass

    obj = bpy.data.objects.new(ase_mesh.name, me)
    bpy.context.collection.objects.link(obj)
    return obj


def _import_export_ase(src: Path, dst: Path):
    # Clear scene
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()

    meshes = parse_ase(src)
    if not meshes:
        raise RuntimeError("no geometry found in ASE")

    for m in meshes:
        _build_blender_mesh(m)

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
        add_leaf_bones=False,
        embed_textures=False,
    )


# ── Conversion loop ───────────────────────────────────────────────────────────

def convert_with_blender(tgl_dir: Path, out_dir: Path):
    groups = group_ase_files(tgl_dir)
    mech_names = _load_mech_names(tgl_dir.parent / "Objects")
    out_dir.mkdir(parents=True, exist_ok=True)
    ok = fail = 0

    for key, group in groups.items():
        display = group["display_name"]
        all_files = group["base"] + group["animation"] + group["lod"] + group["arm"]

        if key in mech_names or any(key.startswith(mn) for mn in mech_names):
            sub = out_dir / "mechs" / display
        elif (tgl_dir / f"{key}.ini").exists():
            sub = out_dir / "vehicles" / display
        else:
            sub = out_dir / "buildings" / display

        sub.mkdir(parents=True, exist_ok=True)

        for ase_path in all_files:
            dst = sub / (ase_path.stem + ".fbx")
            try:
                _import_export_ase(ase_path, dst)
                print(f"  OK  {ase_path.name}")
                ok += 1
            except Exception as e:
                print(f"  FAIL {ase_path.name}: {e}")
                fail += 1

    print(f"\nDone: {ok} converted, {fail} failed")


# ── Manifest mode (no Blender) ────────────────────────────────────────────────

def generate_manifest(tgl_dir: Path, out_path: Path):
    import csv as csv_mod
    groups = group_ase_files(tgl_dir)
    rows = []
    for key, group in groups.items():
        display = group["display_name"]
        for kind in ("base", "animation", "lod", "destroyed", "arm", "prop"):
            for ase in group.get(kind, []):
                rows.append({"ASEFile": str(ase.relative_to(tgl_dir)),
                             "Group": display, "Kind": kind,
                             "TargetFBX": f"{display}/{ase.stem}.fbx"})
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_path, "w", newline="") as f:
        w = csv_mod.DictWriter(f, fieldnames=["ASEFile", "Group", "Kind", "TargetFBX"])
        w.writeheader()
        w.writerows(rows)
    print(f"Manifest: {out_path} ({len(rows)} entries, {len(groups)} groups)")


# ── Entry point ───────────────────────────────────────────────────────────────

def main():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else sys.argv[1:]
    if len(args) < 2:
        print("Usage: blender --background --python ase_to_fbx.py -- <tgl_dir> <out_dir>")
        print("   or: python3 ase_to_fbx.py <tgl_dir> manifest.csv")
        sys.exit(1)

    tgl_dir = Path(args[0])
    out_path = Path(args[1])

    if IN_BLENDER:
        convert_with_blender(tgl_dir, out_path)
    else:
        manifest = out_path if out_path.suffix == ".csv" else out_path / "ase_manifest.csv"
        generate_manifest(tgl_dir, manifest)


if __name__ == "__main__":
    main()
