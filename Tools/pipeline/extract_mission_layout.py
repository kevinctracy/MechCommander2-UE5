"""
extract_mission_layout.py — offline tool (run on your workstation, NOT in UE)
Reads MC2 mission FIT + ABL files and produces one JSON sidecar per mission
for ue_build_mission_level.py to consume.

Usage:
    python extract_mission_layout.py \\
        --missions  Source/Data/Missions \\
        --output    /tmp/mc2_mission_layouts

Output per mission: /tmp/mc2_mission_layouts/mc2_NN.json
Schema documented at the bottom of this file.
"""

import argparse
import json
import re
import struct
from pathlib import Path
from collections import defaultdict


# ── Terrain heightmap selection ───────────────────────────────────────────────
# TerrainMinX magnitude → heightmap file (relative to Source/Data/terrain/)
HEIGHTMAP_BY_EXTENT = {
    6400:  "60x60.tga",
    7680:  "80x80.tga",
    9600:  "100x100.tga",
    12000: "120x120.tga",
}

# BaseTerrain index → biome hint (sky preset name for UE)
BIOME_BY_BASE_TERRAIN = {
    0:  "temperate_clear",
    1:  "temperate_clear",
    2:  "temperate_overcast",
    3:  "arctic_clear",
    22: "desert_clear",
    33: "arctic_night",
    39: "temperate_dusk",
    68: "temperate_storm",
}


def uint_to_rgba(v: int) -> list:
    """Unpack MC2's packed ARGB uint to [r,g,b,a] 0-255."""
    b = v & 0xFF
    g = (v >> 8) & 0xFF
    r = (v >> 16) & 0xFF
    a = (v >> 24) & 0xFF
    return [r, g, b, a]


# ── FIT parsing ───────────────────────────────────────────────────────────────

def parse_fit(path: Path) -> dict:
    """Return { section_name: { key: value_string } }."""
    result   = {}
    section  = None
    kv_re    = re.compile(r'^(?:st|f|l|ul|b|c|uc|d)\s+(\w+)\s*=\s*(.+)$')
    sec_re   = re.compile(r'^\[([^\]]+)\]$')
    for line in path.read_text(encoding='latin-1', errors='replace').splitlines():
        line = line.strip()
        if not line or line.startswith('//'):
            continue
        m = sec_re.match(line)
        if m:
            section = m.group(1)
            result.setdefault(section, {})
            continue
        m = kv_re.match(line)
        if m and section is not None:
            result[section][m.group(1)] = m.group(2).strip().strip('"')
    return result


def extract_terrain(fit: dict) -> dict:
    t = fit.get('Terrain', {})
    try:
        extent = abs(float(t.get('TerrainMinX', -7680)))
    except ValueError:
        extent = 7680.0

    closest = min(HEIGHTMAP_BY_EXTENT.keys(), key=lambda k: abs(k - extent))
    heightmap = HEIGHTMAP_BY_EXTENT[closest]

    try:
        h_min = int(t.get('UserMin', 0))
        h_max = int(t.get('UserMax', 500))
    except ValueError:
        h_min, h_max = 0, 500

    # BaseTerrain appears in [Cameras] section in most FIT files
    base_terrain_raw = t.get('BaseTerrain')
    if not base_terrain_raw:
        for sec_data in fit.values():
            if 'BaseTerrain' in sec_data:
                base_terrain_raw = sec_data['BaseTerrain']
                break
    base_terrain = int(base_terrain_raw or 1)
    biome = BIOME_BY_BASE_TERRAIN.get(base_terrain, 'temperate_clear')

    return {
        'heightmap':    heightmap,
        'extent_m':     closest * 2,          # full width in metres
        'height_min_m': h_min,
        'height_max_m': h_max,
        'base_terrain': base_terrain,
        'biome':        biome,
    }


def extract_weather(fit: dict) -> dict:
    w = fit.get('Weather', {})
    try:
        fog_packed = int(w.get('FogColor', fit.get('3', {}).get('FogColor', 0)) or 0)
    except ValueError:
        fog_packed = 0

    fog_rgba = uint_to_rgba(fog_packed)
    rain     = int(w.get('MaxRainDrops', 0) or 0)
    lightning = float(w.get('BaseLighteningChance', 0) or 0)
    shadow_r = int(fit.get('3', {}).get('TerrainShadowRed', 70) or 70)
    shadow_g = int(fit.get('3', {}).get('TerrainShadowGreen', 70) or 70)
    shadow_b = int(fit.get('3', {}).get('TerrainShadowBlue', 70) or 70)

    return {
        'fog_color_rgba':   fog_rgba,
        'fog_density':      0.02 if fog_packed else 0.0,
        'rain_drops':       rain,
        'has_rain':         rain > 0,
        'lightning_chance': lightning,
        'shadow_color_rgb': [shadow_r, shadow_g, shadow_b],
    }


def extract_mission_meta(fit: dict, mission_id: int) -> dict:
    general = fit.get('3', fit.get('Terrain', {}))
    cameras = fit.get('Cameras', {})
    name = (fit.get('Terrain', {}).get('MissionName')
            or general.get('MissionName')
            or f'mc2_{mission_id:02d}')
    # Strip "mc2_NN" prefix if it's just an ID not a real name
    if re.match(r'^mc2_\d+$', name):
        name = f'Mission {mission_id:02d}'
    return {
        'mission_id': mission_id,
        'name':       name,
        'abl_script': f'mc2_{mission_id:02d}',
    }


# ── ABL parsing: trigger areas ────────────────────────────────────────────────
# Pattern: worldLoc[0] = X; worldLoc[1] = Y; convertCoords(0, worldLoc, cellLoc);
#          VarName = addTriggerArea(cellLoc[0] ± R, cellLoc[1] ± R, ..., type, param);

WORLD_LOC_RE = re.compile(
    r'worldLoc\s*\[\s*0\s*\]\s*=\s*([-\d.]+)\s*;'
    r'.*?worldLoc\s*\[\s*1\s*\]\s*=\s*([-\d.]+)\s*;',
    re.DOTALL
)
ADD_TRIGGER_RE = re.compile(
    r'(\w+)\s*=\s*addTriggerArea\s*\('
    r'\s*cellLoc\s*\[\s*0\s*\]\s*([-+\s\d]*),\s*'
    r'\s*cellLoc\s*\[\s*1\s*\]\s*([-+\s\d]*),\s*'
    r'\s*cellLoc\s*\[\s*0\s*\]\s*([-+\s\d]*),\s*'
    r'\s*cellLoc\s*\[\s*1\s*\]\s*([-+\s\d]*),\s*'
    r'\s*(\d+)\s*,\s*(\d+)\s*\)',
    re.IGNORECASE
)

CELL_SIZE_M = 30.0   # 1 MC2 terrain cell = 30 metres


def eval_offset(expr: str) -> float:
    """Evaluate a simple ± N offset expression like ' - 18' or ' + 12'."""
    expr = expr.strip()
    try:
        return float(expr)
    except ValueError:
        pass
    m = re.match(r'^([+-])\s*(\d+)$', expr)
    if m:
        return float(m.group(1) + m.group(2))
    return 0.0


def extract_trigger_areas(abl_path: Path) -> list:
    """
    Parse an ABL mission script for addTriggerArea calls.
    Returns list of { name, world_x_m, world_y_m, half_width_m, half_height_m, type, param }.
    """
    try:
        text = abl_path.read_text(encoding='latin-1', errors='replace')
    except Exception:
        return []

    # Find all worldLoc assignments followed by addTriggerArea nearby
    areas = []
    # Scan the file for worldLoc[0]/[1] → convertCoords → addTriggerArea sequences
    # We look for worldLoc[0]=X then worldLoc[1]=Y within 10 lines, then addTriggerArea
    lines = text.splitlines()
    i = 0
    pending_x = None
    pending_y = None
    while i < len(lines):
        line = lines[i].strip()
        # Pick up worldLoc assignments
        m = re.match(r'worldLoc\s*\[\s*0\s*\]\s*=\s*([-\d.]+)\s*;', line)
        if m:
            pending_x = float(m.group(1))
        m = re.match(r'worldLoc\s*\[\s*1\s*\]\s*=\s*([-\d.]+)\s*;', line)
        if m:
            pending_y = float(m.group(1))
        # Look for addTriggerArea call (look ahead up to 20 lines from when we have both)
        if pending_x is not None and pending_y is not None:
            for j in range(i, min(i + 20, len(lines))):
                trigger_match = ADD_TRIGGER_RE.search(lines[j])
                if trigger_match:
                    name      = trigger_match.group(1)
                    ul_row_off = eval_offset(trigger_match.group(2))
                    ul_col_off = eval_offset(trigger_match.group(3))
                    lr_row_off = eval_offset(trigger_match.group(4))
                    lr_col_off = eval_offset(trigger_match.group(5))
                    area_type  = int(trigger_match.group(6))
                    param      = int(trigger_match.group(7))

                    # Cell offsets to metres
                    ul_x = pending_x + ul_row_off * CELL_SIZE_M
                    ul_y = pending_y + ul_col_off * CELL_SIZE_M
                    lr_x = pending_x + lr_row_off * CELL_SIZE_M
                    lr_y = pending_y + lr_col_off * CELL_SIZE_M

                    center_x = (ul_x + lr_x) / 2.0
                    center_y = (ul_y + lr_y) / 2.0
                    half_w   = abs(lr_x - ul_x) / 2.0
                    half_h   = abs(lr_y - ul_y) / 2.0

                    areas.append({
                        'name':       name,
                        'center_x_m': center_x,
                        'center_y_m': center_y,
                        'half_w_m':   half_w,
                        'half_h_m':   half_h,
                        'type':       area_type,
                        'param':      param,
                    })
                    pending_x = None
                    pending_y = None
                    break
        i += 1

    return areas


# ── Main ─────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description='Extract MC2 mission layout to JSON')
    parser.add_argument('--missions', required=True, help='Path to Source/Data/Missions/')
    parser.add_argument('--output',   required=True, help='Output directory for JSON files')
    args = parser.parse_args()

    missions_dir = Path(args.missions)
    output_dir   = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    fit_files = sorted(missions_dir.glob('mc2_[0-9]*.fit'))
    for fit_path in fit_files:
        m = re.search(r'mc2_(\d+)', fit_path.stem, re.IGNORECASE)
        if not m:
            continue
        mission_id = int(m.group(1))

        fit   = parse_fit(fit_path)
        meta    = extract_mission_meta(fit, mission_id)
        terrain = extract_terrain(fit)
        weather = extract_weather(fit)

        abl_path = fit_path.with_suffix('.abl')
        trigger_areas = extract_trigger_areas(abl_path) if abl_path.exists() else []

        layout = {
            **meta,
            'terrain':       terrain,
            'weather':       weather,
            'trigger_areas': trigger_areas,
        }

        out_path = output_dir / f'mc2_{mission_id:02d}.json'
        with open(out_path, 'w') as f:
            json.dump(layout, f, indent=2)

        print(f'[extract_layout] mc2_{mission_id:02d}: '
              f'biome={terrain["biome"]}  '
              f'heightmap={terrain["heightmap"]}  '
              f'rain={weather["has_rain"]}  '
              f'triggers={len(trigger_areas)}')

    print(f'[extract_layout] Done — {len(fit_files)} missions written to {output_dir}')


if __name__ == '__main__':
    main()


# ── Output JSON schema ────────────────────────────────────────────────────────
# {
#   "mission_id":  18,
#   "name":        "The City",
#   "abl_script":  "mc2_18",
#   "terrain": {
#     "heightmap":    "80x80.tga",       // file in Source/Data/terrain/
#     "extent_m":     15360,             // full map width/height in metres
#     "height_min_m": 0,
#     "height_max_m": 500,
#     "base_terrain": 1,
#     "biome":        "temperate_clear"  // sky/lighting preset name
#   },
#   "weather": {
#     "fog_color_rgba":   [200, 200, 200, 0],
#     "fog_density":      0.02,
#     "rain_drops":       0,
#     "has_rain":         false,
#     "lightning_chance": 0.0,
#     "shadow_color_rgb": [31, 31, 31]
#   },
#   "trigger_areas": [
#     {
#       "name":       "sneakyAreaTrigger",
#       "center_x_m": 5482.0,
#       "center_y_m": 4580.0,
#       "half_w_m":   540.0,    // 18 cells × 30m
#       "half_h_m":   540.0,
#       "type":       2,
#       "param":      0
#     }
#   ]
# }
