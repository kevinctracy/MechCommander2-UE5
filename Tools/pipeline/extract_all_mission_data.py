"""
extract_all_mission_data.py — offline tool (run on your workstation, NOT in UE)
Single-pass extraction of all MC2 mission data into one JSON per mission.

Replaces: extract_mission_data.py, extract_mission_layout.py, extract_patrol_paths.py

Usage:
    python extract_all_mission_data.py \\
        --source   Source/Data \\
        --warriors Source/Data/Missions/Warriors \\
        --output   /tmp/mc2_missions

Output per mission: /tmp/mc2_missions/mc2_NN.json

Schema documented at bottom of file.
"""

import argparse
import json
import re
from collections import defaultdict
from pathlib import Path


# ── Object type classification ────────────────────────────────────────────────
# ObjectTypeNum from Objects/*.fit:  1=dropship  2=mech/copter  3=vehicle  11/18=static prop
# Units whose CSVFile stem matches a .csv in Objects/ are BattleMechs (type 2).

# Explicit overrides for CSVFile names that lack a matching .fit in Objects/
# (verified by checking Objects/*.csv — presence of MechName row = mech)
_MECH_CSVFILES = {
    "anubis", "atlas", "bloodasp", "bushwacker", "catapult", "cyclops",
    "enfield", "highlander", "hollander", "hunchback", "jaegermech",
    "laohu", "madcat", "mediumcopter", "menshen", "raven", "shayu",
    "shootist", "starslayer", "uller", "vulture", "werewolf", "wolfhound",
    "zeus",
}
# CSVFile names that are vehicles but have no .fit (or wrong .fit) in Objects/
_VEHICLE_CSVFILES = {
    "lrmc", "legion", "mog", "monsoon", "storm", "trooptransport",
}

# ObjectTypeNum → canonical UE actor kind
_TYPENUM_KIND = {
    0:  "prop",
    1:  "dropship",
    2:  "mech",
    3:  "vehicle",
    4:  "vehicle",
    7:  "vehicle",
    11: "prop",
    18: "prop",
    21: "building",  # powered/spotlight structures
    22: "building",  # gates
    23: "turret",
    24: "building",
}

# Biome presets (sky/lighting preset name for UE)
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

# Terrain extent → heightmap TGA (relative to Source/Data/terrain/)
HEIGHTMAP_BY_EXTENT = {
    6400:  "60x60.tga",
    7680:  "80x80.tga",
    9600:  "100x100.tga",
    12000: "120x120.tga",
}

CELL_SIZE_M = 30.0  # 1 MC2 terrain cell = 30 metres


# ── Object catalogue: build CSVFile → kind ───────────────────────────────────

def build_object_catalogue(objects_dir: Path) -> dict[str, str]:
    """Return {csv_file_stem_lower: kind_string} from Objects/*.fit."""
    typenum_re = re.compile(r'ObjectTypeNum\s*=\s*(\d+)', re.IGNORECASE)
    catalogue: dict[str, str] = {}

    for f in list(objects_dir.glob('*.fit')) + list(objects_dir.glob('*.FIT')):
        txt = f.read_text(encoding='latin-1', errors='replace')
        m = typenum_re.search(txt)
        if m:
            kind = _TYPENUM_KIND.get(int(m.group(1)), 'prop')
            catalogue[f.stem.lower()] = kind

    # Apply explicit overrides
    for name in _MECH_CSVFILES:
        catalogue.setdefault(name, 'mech')
    for name in _VEHICLE_CSVFILES:
        catalogue.setdefault(name, 'vehicle')

    return catalogue


# ── FIT parser ────────────────────────────────────────────────────────────────

def parse_fit(path: Path) -> dict[str, dict[str, str]]:
    """Return {section_name: {key: value}} for a FIT file."""
    result: dict[str, dict[str, str]] = {}
    section: str | None = None
    kv_re  = re.compile(r'^(?:st|f|l|ul|b|c|uc|d)\s+(\w+)\s*=\s*(.+)$')
    sec_re = re.compile(r'^\[([^\]]+)\]$')
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


# ── Terrain / weather / biome extraction ─────────────────────────────────────

def extract_terrain(fit: dict) -> dict:
    t = fit.get('Terrain', {})
    try:
        extent = abs(float(t.get('TerrainMinX', -7680)))
    except ValueError:
        extent = 7680.0

    closest = min(HEIGHTMAP_BY_EXTENT, key=lambda k: abs(k - extent))
    heightmap = HEIGHTMAP_BY_EXTENT[closest]

    try:
        h_min = int(t.get('UserMin', 0))
        h_max = int(t.get('UserMax', 500))
    except ValueError:
        h_min, h_max = 0, 500

    # BaseTerrain lives in [Cameras] in most FIT files — scan all sections
    base_terrain_raw = None
    for sec_data in fit.values():
        if 'BaseTerrain' in sec_data:
            base_terrain_raw = sec_data['BaseTerrain']
            break
    base_terrain = int(base_terrain_raw or 1)
    biome = BIOME_BY_BASE_TERRAIN.get(base_terrain, 'temperate_clear')

    return {
        'heightmap':    heightmap,
        'extent_m':     closest * 2,
        'height_min_m': h_min,
        'height_max_m': h_max,
        'base_terrain': base_terrain,
        'biome':        biome,
    }


def uint_to_rgba(v: int) -> list:
    return [(v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF, (v >> 24) & 0xFF]


def extract_weather(fit: dict) -> dict:
    w = fit.get('Weather', {})
    try:
        fog_packed = int(w.get('FogColor', fit.get('3', {}).get('FogColor', 0)) or 0)
    except ValueError:
        fog_packed = 0

    fog_rgba  = uint_to_rgba(fog_packed)
    rain      = int(w.get('MaxRainDrops', 0) or 0)
    lightning = float(w.get('BaseLighteningChance', 0) or 0)
    sec3      = fit.get('3', {})
    shadow_r  = int(sec3.get('TerrainShadowRed',   70) or 70)
    shadow_g  = int(sec3.get('TerrainShadowGreen', 70) or 70)
    shadow_b  = int(sec3.get('TerrainShadowBlue',  70) or 70)

    return {
        'fog_color_rgba':   fog_rgba,
        'fog_density':      0.02 if fog_packed else 0.0,
        'rain_drops':       rain,
        'has_rain':         rain > 0,
        'lightning_chance': lightning,
        'shadow_color_rgb': [shadow_r, shadow_g, shadow_b],
    }


# ── Mission meta ──────────────────────────────────────────────────────────────

def extract_meta(fit: dict, mission_id: int) -> dict:
    name = (fit.get('Terrain', {}).get('MissionName')
            or fit.get('3', {}).get('MissionName')
            or f'Mission {mission_id:02d}')
    if re.match(r'^mc2_\d+$', name):
        name = f'Mission {mission_id:02d}'
    return {
        'mission_id': mission_id,
        'name':       name,
        'abl_script': f'mc2_{mission_id:02d}',
    }


# ── Unit actor extraction ─────────────────────────────────────────────────────

def extract_actors(fit: dict, catalogue: dict[str, str]) -> list:
    """
    Extract all Part sections with correct field names.
    Fields used: PositionX, PositionY, Rotation, TeamID, CSVFile, Pilot (warrior link).
    """
    # Build warrior_id → brain mapping
    warrior_brain: dict[int, str] = {}
    for sec, data in fit.items():
        m = re.match(r'^Warrior(\d+)$', sec)
        if m and 'Brain' in data:
            warrior_brain[int(m.group(1))] = data['Brain']

    actors = []
    for sec, data in fit.items():
        m = re.match(r'^Part(\d+)$', sec)
        if not m:
            continue
        part_id = int(m.group(1))

        csv_file = data.get('CSVFile', '').strip('"')
        if not csv_file:
            continue

        kind = catalogue.get(csv_file.lower(), 'vehicle')

        def fl(k, default=0.0):
            try:
                return float(data.get(k, default))
            except (ValueError, TypeError):
                return default

        def it(k, default=0):
            try:
                return int(data.get(k, default))
            except (ValueError, TypeError):
                return default

        pilot_id = it('Pilot', -1)
        brain    = warrior_brain.get(part_id, '')

        actors.append({
            'part_id':      part_id,
            'kind':         kind,
            'csv_file':     csv_file,
            'position_m':   [fl('PositionX'), fl('PositionY')],
            'rotation_deg': fl('Rotation'),
            'team_id':      it('TeamID'),
            'pilot_id':     pilot_id,
            'brain':        brain,
            'variant':      it('VariantNumber'),
            'squad':        it('SquadNum', -1),
        })

    return actors


# ── Objectives ────────────────────────────────────────────────────────────────

def extract_objectives(fit: dict) -> list:
    objectives = []
    for sec, data in fit.items():
        if not sec.lower().startswith('objective'):
            continue
        m = re.search(r'\d+', sec)
        obj_id = int(m.group()) if m else 0
        objectives.append({
            'id':      obj_id,
            'text':    data.get('DisplayText', data.get('displayText', data.get('text', ''))),
            'required': data.get('Required', data.get('required', 'true')).lower() == 'true',
        })
    return objectives


# ── Trigger areas (from mission ABL) ─────────────────────────────────────────

ADD_TRIGGER_RE = re.compile(
    r'(\w+)\s*=\s*addTriggerArea\s*\('
    r'\s*cellLoc\s*\[\s*0\s*\]\s*([-+\s\d]*),\s*'
    r'\s*cellLoc\s*\[\s*1\s*\]\s*([-+\s\d]*),\s*'
    r'\s*cellLoc\s*\[\s*0\s*\]\s*([-+\s\d]*),\s*'
    r'\s*cellLoc\s*\[\s*1\s*\]\s*([-+\s\d]*),\s*'
    r'\s*(\d+)\s*,\s*(\d+)\s*\)',
    re.IGNORECASE
)


def _eval_offset(expr: str) -> float:
    expr = expr.strip()
    try:
        return float(expr)
    except ValueError:
        pass
    m = re.match(r'^([+-])\s*(\d+)$', expr)
    return float(m.group(1) + m.group(2)) if m else 0.0


def extract_trigger_areas(abl_path: Path) -> list:
    try:
        lines = abl_path.read_text(encoding='latin-1', errors='replace').splitlines()
    except Exception:
        return []

    areas = []
    pending_x: float | None = None
    pending_y: float | None = None

    for i, line in enumerate(lines):
        s = line.strip()
        m = re.match(r'worldLoc\s*\[\s*0\s*\]\s*=\s*([-\d.]+)\s*;', s)
        if m:
            pending_x = float(m.group(1))
        m = re.match(r'worldLoc\s*\[\s*1\s*\]\s*=\s*([-\d.]+)\s*;', s)
        if m:
            pending_y = float(m.group(1))

        if pending_x is not None and pending_y is not None:
            for j in range(i, min(i + 20, len(lines))):
                tm = ADD_TRIGGER_RE.search(lines[j])
                if tm:
                    ul_x = pending_x + _eval_offset(tm.group(2)) * CELL_SIZE_M
                    ul_y = pending_y + _eval_offset(tm.group(3)) * CELL_SIZE_M
                    lr_x = pending_x + _eval_offset(tm.group(4)) * CELL_SIZE_M
                    lr_y = pending_y + _eval_offset(tm.group(5)) * CELL_SIZE_M
                    areas.append({
                        'name':       tm.group(1),
                        'center_x_m': (ul_x + lr_x) / 2.0,
                        'center_y_m': (ul_y + lr_y) / 2.0,
                        'half_w_m':   abs(lr_x - ul_x) / 2.0,
                        'half_h_m':   abs(lr_y - ul_y) / 2.0,
                        'type':       int(tm.group(6)),
                        'param':      int(tm.group(7)),
                    })
                    pending_x = None
                    pending_y = None
                    break

    return areas


# ── Patrol paths (from warrior ABL brains) ───────────────────────────────────

PATROL_STATE_RE = re.compile(r'startPatrolState\s*\[\s*(\d)\s*\]\s*=\s*([^;]+);')
PATROL_PATH_RE  = re.compile(r'startPatrolPath\s*\[\s*(\d+)\s*,\s*([01])\s*\]\s*=\s*([-\d.]+)\s*;')
FSM_NAME_RE     = re.compile(r'^\s*fsm\s+(\w+)\s*;', re.MULTILINE)


def parse_patrol_brain(path: Path) -> dict | None:
    try:
        text = path.read_text(encoding='latin-1', errors='replace')
    except Exception:
        return None

    if 'PatrolPath' not in text and 'patrolPath' not in text:
        return None

    m = FSM_NAME_RE.search(text)
    brain_name = m.group(1) if m else path.stem

    patrol_type = 'linear'
    num_cycles  = -1
    for m in PATROL_STATE_RE.finditer(text):
        idx, val = int(m.group(1)), m.group(2).strip()
        if idx == 0:
            patrol_type = 'looping' if 'LOOPING' in val.upper() else 'linear'
        elif idx == 2:
            try:
                num_cycles = int(float(val))
            except ValueError:
                num_cycles = -1

    raw: dict[int, dict[int, float]] = defaultdict(dict)
    for m in PATROL_PATH_RE.finditer(text):
        pt_idx, axis, val = int(m.group(1)), int(m.group(2)), float(m.group(3))
        raw[pt_idx][axis] = val

    if not raw:
        return None

    waypoints = [[raw[i].get(0, 0.0), raw[i].get(1, 0.0)] for i in sorted(raw)]
    return {
        'brain_name':  brain_name,
        'patrol_type': patrol_type,
        'num_cycles':  num_cycles,
        'waypoints':   waypoints,
    }


def load_all_brains(warriors_dir: Path) -> dict[str, dict]:
    """Return {brain_name_lower: brain_data} from all ABL files under warriors_dir."""
    brains: dict[str, dict] = {}
    for abl in list(warriors_dir.rglob('*.abl')) + list(warriors_dir.rglob('*.ABL')):
        data = parse_patrol_brain(abl)
        if data:
            brains[data['brain_name'].lower()] = data
    return brains


def extract_patrol_chains(fit: dict, mission_id: int, brains: dict) -> list:
    """
    Cross-reference FIT Part/Warrior sections with brain data to produce patrol chains.
    """
    warrior_brain: dict[int, str] = {}
    for sec, data in fit.items():
        m = re.match(r'^Warrior(\d+)$', sec)
        if m and 'Brain' in data:
            warrior_brain[int(m.group(1))] = data['Brain']

    chains: dict[str, dict] = {}

    for sec, data in fit.items():
        m = re.match(r'^Part(\d+)$', sec)
        if not m:
            continue
        part_id = int(m.group(1))
        brain_name = warrior_brain.get(part_id, '')
        if not brain_name:
            continue
        brain_lower = brain_name.lower()
        if brain_lower not in brains:
            continue

        try:
            pos_x   = float(data.get('PositionX', 0))
            pos_y   = float(data.get('PositionY', 0))
            team_id = int(data.get('TeamID', -1))
        except ValueError:
            continue

        if brain_lower not in chains:
            bd = brains[brain_lower]
            chains[brain_lower] = {
                'brain_name':  bd['brain_name'],
                'mission_id':  mission_id,
                'patrol_type': bd['patrol_type'],
                'num_cycles':  bd['num_cycles'],
                'team_id':     team_id,
                'units':       [],
                'waypoints':   bd['waypoints'],
            }

        chains[brain_lower]['units'].append({
            'part_id': part_id,
            'spawn':   [pos_x, pos_y, 0.0],
        })

    return sorted(chains.values(), key=lambda c: c['brain_name'])


# ── Main ─────────────────────────────────────────────────────────────────────

def extract_mission(fit_path: Path, mission_id: int,
                    catalogue: dict, brains: dict, output_dir: Path):
    fit = parse_fit(fit_path)

    meta         = extract_meta(fit, mission_id)
    terrain      = extract_terrain(fit)
    weather      = extract_weather(fit)
    actors       = extract_actors(fit, catalogue)
    objectives   = extract_objectives(fit)
    patrol_chains = extract_patrol_chains(fit, mission_id, brains)

    abl_path     = fit_path.with_suffix('.abl')
    if not abl_path.exists():
        abl_path = fit_path.with_suffix('.ABL')
    trigger_areas = extract_trigger_areas(abl_path) if abl_path.exists() else []

    layout = {
        **meta,
        'terrain':        terrain,
        'weather':        weather,
        'actors':         actors,
        'objectives':     objectives,
        'trigger_areas':  trigger_areas,
        'patrol_chains':  patrol_chains,
    }

    out_path = output_dir / f'mc2_{mission_id:02d}.json'
    with open(out_path, 'w') as f:
        json.dump(layout, f, indent=2)

    mech_count    = sum(1 for a in actors if a['kind'] == 'mech')
    vehicle_count = sum(1 for a in actors if a['kind'] == 'vehicle')
    print(f'[extract] mc2_{mission_id:02d}  '
          f'biome={terrain["biome"]:22s}  '
          f'mechs={mech_count:3d}  vehicles={vehicle_count:3d}  '
          f'triggers={len(trigger_areas):2d}  patrols={len(patrol_chains):2d}')


def main():
    parser = argparse.ArgumentParser(description='Extract MC2 mission data to JSON')
    parser.add_argument('--source',   required=True, help='Path to Source/Data/')
    parser.add_argument('--warriors', required=True, help='Path to Source/Data/Missions/Warriors/')
    parser.add_argument('--output',   required=True, help='Output directory for JSON files')
    parser.add_argument('--mission',  type=int,      help='Extract single mission number')
    args = parser.parse_args()

    source_dir   = Path(args.source)
    warriors_dir = Path(args.warriors)
    output_dir   = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    objects_dir  = source_dir / 'Objects'
    missions_dir = source_dir / 'Missions'

    catalogue = build_object_catalogue(objects_dir) if objects_dir.exists() else {}
    brains    = load_all_brains(warriors_dir) if warriors_dir.exists() else {}

    print(f'[extract] Loaded {len(catalogue)} object types, {len(brains)} patrol brains')

    fits = sorted(missions_dir.glob('mc2_[0-9]*.fit'))
    if args.mission is not None:
        fits = [f for f in fits
                if (m := re.search(r'\d+', f.stem)) and int(m.group()) == args.mission]

    if not fits:
        print(f'[extract] No mc2_NN.fit files found in {missions_dir}')
        return

    for fit_path in fits:
        m = re.search(r'mc2_(\d+)', fit_path.stem, re.IGNORECASE)
        if not m:
            continue
        mission_id = int(m.group(1))
        extract_mission(fit_path, mission_id, catalogue, brains, output_dir)

    print(f'[extract] Done — {len(fits)} missions written to {output_dir}')


if __name__ == '__main__':
    main()


# ── Output JSON schema ────────────────────────────────────────────────────────
# {
#   "mission_id":  18,
#   "name":        "The City",
#   "abl_script":  "mc2_18",
#
#   "terrain": {
#     "heightmap":    "80x80.tga",
#     "extent_m":     15360,
#     "height_min_m": 0,
#     "height_max_m": 500,
#     "base_terrain": 1,
#     "biome":        "temperate_clear"
#   },
#
#   "weather": {
#     "fog_color_rgba":   [200, 200, 200, 0],
#     "fog_density":      0.02,
#     "rain_drops":       0,
#     "has_rain":         false,
#     "lightning_chance": 0.0,
#     "shadow_color_rgb": [31, 31, 31]
#   },
#
#   "actors": [
#     {
#       "part_id":      47,
#       "kind":         "mech",         // "mech"|"vehicle"|"building"|"turret"|"dropship"|"prop"
#       "csv_file":     "Werewolf",
#       "position_m":   [2228.96, -2076.07],  // [x, y] in metres (MC2 world space)
#       "rotation_deg": 180.0,
#       "team_id":      1,
#       "pilot_id":     2,              // -1 if no pilot
#       "brain":        "mc2_18_patrol_1",  // "" if no warrior brain
#       "variant":      0,
#       "squad":        2
#     }
#   ],
#
#   "objectives": [
#     { "id": 0, "text": "Destroy the factory", "required": true }
#   ],
#
#   "trigger_areas": [
#     {
#       "name":       "sneakyAreaTrigger",
#       "center_x_m": 5482.0,
#       "center_y_m": 4580.0,
#       "half_w_m":   540.0,
#       "half_h_m":   540.0,
#       "type":       2,
#       "param":      0
#     }
#   ],
#
#   "patrol_chains": [
#     {
#       "brain_name":  "mc2_18_patrol_1",
#       "mission_id":  18,
#       "patrol_type": "linear",        // "linear" (ping-pong) | "looping" (circular)
#       "num_cycles":  -1,
#       "team_id":     1,
#       "units":       [ { "part_id": 47, "spawn": [1984.0, 319.9, 0.0] } ],
#       "waypoints":   [ [1984.0, 320.0], [3904.0, 149.0] ]
#     }
#   ]
# }
