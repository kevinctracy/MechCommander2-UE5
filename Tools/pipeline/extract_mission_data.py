"""
extract_mission_data.py — offline tool (run on your workstation, NOT in UE)
Converts MC2 mission FIT / ODF files into JSON sidecars for ue_place_mission_actors.py.

Usage:
    python extract_mission_data.py \\
        --source  /path/to/Source/Data \\
        --output  /tmp/mc2_missions \\
        [--mission 1]        # optional: single mission number

What it reads
-------------
Source/Data/Missions/mc2_NN.fit   — unit placement records (Part entries)
Source/Data/ODF/                  — object definition files for type lookup
Source/Data/Terrain/              — .trn files for map dimensions (unused by actor placer)

Output per mission:
    MC2_Mission_NN.json   — array of actor records + objectives array

The coordinate system from MC2 FIT files is:
    x, y  — map tile coordinates (1 tile = 30 metres = 3000 UU)
    z      — altitude in metres above sea level

This script converts to UE centimetres and flips Z: see mc2_to_ue() in ue_place_mission_actors.py.
"""

import argparse
import json
import os
import re
from pathlib import Path

# Minimal FIT key-value parser (same logic as fit_convert.py)
TYPE_PREFIX_RE = re.compile(r'^(l|f|st|uc|b|c|d)\s+(\S+)\s+(.+)$')

def parse_fit(path: Path) -> dict:
    """Parse a FIT file into a dict of dicts keyed by [section][key]."""
    result   = {}
    section  = None
    with open(path, "r", encoding="latin-1", errors="replace") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("//") or line.startswith(";"):
                continue
            if line.startswith("[") and line.endswith("]"):
                section = line[1:-1]
                result.setdefault(section, {})
                continue
            m = TYPE_PREFIX_RE.match(line)
            if m and section is not None:
                _, key, val = m.groups()
                result[section][key] = val.strip().strip('"')
            elif "=" in line and section is not None:
                key, _, val = line.partition("=")
                result[section][key.strip()] = val.strip().strip('"')
    return result


def section_to_actor(section: dict, odf_index: dict) -> dict | None:
    """Convert a FIT Part section to an actor record."""
    obj_type = section.get("objectType", section.get("ObjectType", "")).lower()
    if not obj_type:
        return None

    # Map MC2 object types to UE Blueprint names
    TYPE_MAP = {
        "battlemech": "mech",
        "mech":       "mech",
        "groundvehicle": "building",   # placed as static for now
        "vehicle":    "building",
        "building":   "building",
        "turret":     "turret",
        "tree":       "building",
        "objective":  "objective",
    }

    ue_type = TYPE_MAP.get(obj_type, "building")

    def fl(k, default=0.0):
        try:
            return float(section.get(k, default))
        except ValueError:
            return default

    def it(k, default=0):
        try:
            return int(section.get(k, default))
        except ValueError:
            return default

    pos_x = fl("x", 0.0)
    pos_y = fl("y", 0.0)
    pos_z = fl("z", 0.0)

    # MC2 tile → world metres: 1 tile = 30 m; convert to cm for UE
    TILE_M = 30.0
    actor = {
        "type":         ue_type,
        "blueprint":    f"BP_{ue_type.capitalize()}",
        "team":         it("teamIndex", 0),
        "position":     [pos_x * TILE_M, pos_z, pos_y * TILE_M],  # world metres
        "rotation_yaw": fl("rotation", 0.0),
        "variant":      section.get("chassisID", section.get("ChassisID", "")),
        "label":        section.get("label", section.get("partID", "")),
    }
    return actor


def extract_mission(fit_path: Path, odf_index: dict, output_dir: Path):
    stem = fit_path.stem   # e.g. mc2_01
    match = re.search(r'(\d+)', stem)
    mission_num = int(match.group(1)) if match else 0

    fit = parse_fit(fit_path)

    actors     = []
    objectives = []

    for section_name, section_data in fit.items():
        lower = section_name.lower()

        if lower.startswith("part"):
            actor = section_to_actor(section_data, odf_index)
            if actor:
                actors.append(actor)

        elif lower.startswith("objective"):
            objectives.append({
                "id":           int(re.search(r'\d+', section_name).group() or 0),
                "text":         section_data.get("text", section_data.get("displayText", "")),
                "required":     section_data.get("required", "true").lower() == "true",
                "trigger_actor": section_data.get("triggerLabel", ""),
            })

    output = {
        "mission_id": mission_num,
        "name":       fit.get("MissionInfo", {}).get("name", stem),
        "actors":     actors,
        "objectives": objectives,
    }

    out_path = output_dir / f"MC2_Mission_{mission_num:02d}.json"
    with open(out_path, "w") as f:
        json.dump(output, f, indent=2)

    print(f"[extract] {stem}: {len(actors)} actors, {len(objectives)} objectives → {out_path.name}")


def build_odf_index(source_dir: Path) -> dict:
    """Build a quick name→type lookup from ODF files."""
    index = {}
    odf_dir = source_dir / "ODF"
    if not odf_dir.exists():
        return index
    for odf in odf_dir.rglob("*.odf"):
        try:
            content = odf.read_text(encoding="latin-1", errors="replace")
            for line in content.splitlines():
                m = re.match(r'\s*objectType\s*=\s*"?(\w+)"?', line, re.IGNORECASE)
                if m:
                    index[odf.stem.lower()] = m.group(1).lower()
                    break
        except Exception:
            pass
    return index


def main():
    parser = argparse.ArgumentParser(description="Extract MC2 mission data to JSON")
    parser.add_argument("--source",  required=True, help="Path to Source/Data directory")
    parser.add_argument("--output",  required=True, help="Output directory for JSON files")
    parser.add_argument("--mission", type=int,      help="Extract single mission number only")
    args = parser.parse_args()

    source_dir = Path(args.source)
    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    odf_index  = build_odf_index(source_dir)
    mission_dir = source_dir / "Missions"
    if not mission_dir.exists():
        # Try alternative locations
        for candidate in [source_dir / "ABL", source_dir, source_dir.parent]:
            fits = list(candidate.glob("mc2_*.fit"))
            if fits:
                mission_dir = candidate
                break

    fits = sorted(mission_dir.glob("mc2_*.fit"))
    if args.mission:
        fits = [f for f in fits if re.search(r'\d+', f.stem) and
                int(re.search(r'\d+', f.stem).group()) == args.mission]

    if not fits:
        print(f"[extract] No mc2_NN.fit files found in {mission_dir}")
        return

    for fit_path in fits:
        extract_mission(fit_path, odf_index, output_dir)

    print(f"[extract] Done. {len(fits)} missions written to {output_dir}")


if __name__ == "__main__":
    main()
