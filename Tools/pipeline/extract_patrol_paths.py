"""
extract_patrol_paths.py — offline tool (run on your workstation, NOT in UE)
Extracts patrol waypoint chains from MC2 warrior ABL brain scripts and mission FIT files.

Usage:
    python extract_patrol_paths.py \\
        --warriors  Source/Data/Missions/Warriors \\
        --missions  Source/Data/Missions \\
        --output    /tmp/mc2_patrol_paths.json

Output JSON schema:
{
  "chains": [
    {
      "brain_name":    "mc2_18_patrol_1",
      "mission_id":    18,
      "patrol_type":   "linear",        // "linear" (ping-pong) | "looping" (circular)
      "num_cycles":    -1,              // -1 = infinite
      "team_id":       1,
      "units": [                        // all Part instances that use this brain
        { "part_id": 47, "spawn": [1984.0, 319.9, 0.0] }
      ],
      "waypoints": [
        [1984.0, 320.0],               // [world_x, world_y] in metres
        [3904.0, 149.0]
      ]
    }
  ]
}

Coordinate system note:
  MC2 world X,Y are in metres. UE conversion (done in ue_place_waypoints.py):
    UE_X =  mc2_x * 100  (cm)
    UE_Y = -mc2_y * 100  (cm, axis flip)
    UE_Z =  0            (Z from landscape height query in UE)
"""

import argparse
import json
import os
import re
from pathlib import Path
from collections import defaultdict

# ── ABL parsing ─────────────────────────────────────────────────────────────

PATROL_STATE_RE = re.compile(
    r'startPatrolState\s*\[\s*(\d)\s*\]\s*=\s*([^;]+);'
)
PATROL_PATH_RE = re.compile(
    r'startPatrolPath\s*\[\s*(\d+)\s*,\s*([01])\s*\]\s*=\s*([-\d.]+)\s*;'
)
FSM_NAME_RE   = re.compile(r'^\s*fsm\s+(\w+)\s*;', re.MULTILINE)


def parse_patrol_brain(path: Path) -> dict | None:
    """
    Parse a single warrior ABL file.  Returns None if no PatrolPath found.
    """
    try:
        text = path.read_text(encoding='latin-1', errors='replace')
    except Exception:
        return None

    # Only process files that actually define a patrol path
    if 'PatrolPath' not in text and 'patrolPath' not in text:
        return None

    # Brain (fsm) name
    m = FSM_NAME_RE.search(text)
    brain_name = m.group(1) if m else path.stem

    # Patrol state array
    patrol_type  = 'linear'   # default
    num_cycles   = -1
    for m in PATROL_STATE_RE.finditer(text):
        idx, val = int(m.group(1)), m.group(2).strip()
        if idx == 0:
            patrol_type = 'looping' if 'LOOPING' in val.upper() else 'linear'
        elif idx == 2:
            try:
                num_cycles = int(float(val))
            except ValueError:
                num_cycles = -1

    # Patrol path points: [point_index, axis] = value
    raw: dict[int, dict[int, float]] = defaultdict(dict)
    for m in PATROL_PATH_RE.finditer(text):
        pt_idx, axis, val = int(m.group(1)), int(m.group(2)), float(m.group(3))
        raw[pt_idx][axis] = val

    if not raw:
        return None

    waypoints = []
    for pt_idx in sorted(raw.keys()):
        axes = raw[pt_idx]
        x = axes.get(0, 0.0)
        y = axes.get(1, 0.0)
        waypoints.append([x, y])

    return {
        'brain_name':  brain_name,
        'patrol_type': patrol_type,
        'num_cycles':  num_cycles,
        'waypoints':   waypoints,
    }


# ── FIT parsing ──────────────────────────────────────────────────────────────

def parse_fit_warriors(fit_path: Path) -> dict:
    """
    Extract warrior→brain and part→(position, team, brain) mappings from a FIT file.
    Returns { warrior_id: { brain, part_id, pos_x, pos_y, team_id } }
    """
    try:
        text = fit_path.read_text(encoding='latin-1', errors='replace')
    except Exception:
        return {}

    # Split into sections [SectionName]\n key=val...
    section_re  = re.compile(r'^\[(\w+)\]', re.MULTILINE)
    kv_re       = re.compile(r'^(?:st|f|l|ul|b|c|uc|d)\s+(\w+)\s*=\s*(.+)$', re.MULTILINE)

    sections = {}
    positions = [m.start() for m in section_re.finditer(text)]
    for i, pos in enumerate(positions):
        name = section_re.match(text, pos).group(1)
        end  = positions[i + 1] if i + 1 < len(positions) else len(text)
        body = text[pos:end]
        kvs  = {m.group(1): m.group(2).strip().strip('"') for m in kv_re.finditer(body)}
        sections[name] = kvs

    # warrior_id → brain
    warrior_brain: dict[int, str] = {}
    for name, kvs in sections.items():
        m = re.match(r'^Warrior(\d+)$', name)
        if m and 'Brain' in kvs:
            warrior_brain[int(m.group(1))] = kvs['Brain']

    # part_id → data (parts share ID with their warrior)
    result = {}
    for name, kvs in sections.items():
        m = re.match(r'^Part(\d+)$', name)
        if not m:
            continue
        part_id = int(m.group(1))
        brain   = warrior_brain.get(part_id)
        if not brain:
            continue
        try:
            pos_x   = float(kvs.get('PositionX', 0))
            pos_y   = float(kvs.get('PositionY', 0))
            team_id = int(kvs.get('TeamID', kvs.get('teamID', -1)))
        except ValueError:
            continue
        result[part_id] = {
            'brain':   brain,
            'part_id': part_id,
            'pos_x':   pos_x,
            'pos_y':   pos_y,
            'team_id': team_id,
        }

    return result


# ── Main ─────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description='Extract MC2 patrol paths to JSON')
    parser.add_argument('--warriors', required=True, help='Path to Warriors/ directory')
    parser.add_argument('--missions', required=True, help='Path to Missions/ directory')
    parser.add_argument('--output',   required=True, help='Output JSON file path')
    args = parser.parse_args()

    warriors_dir = Path(args.warriors)
    missions_dir = Path(args.missions)
    output_path  = Path(args.output)

    # 1. Parse all brain ABL files
    brains: dict[str, dict] = {}
    for abl in warriors_dir.rglob('*.abl'):
        data = parse_patrol_brain(abl)
        if data:
            brains[data['brain_name'].lower()] = data

    # Also try case-insensitive match by filename stem
    for abl in warriors_dir.rglob('*.ABL'):
        data = parse_patrol_brain(abl)
        if data:
            brains[data['brain_name'].lower()] = data

    print(f'[extract_patrol] Parsed {len(brains)} patrol brain files.')

    # 2. Parse mission FIT files, group units by brain
    # chain_key = (brain_name_lower, mission_id)
    chains: dict[tuple, dict] = {}

    fit_files = sorted(missions_dir.glob('mc2_*.fit')) + sorted(missions_dir.glob('mc2_*.FIT'))
    for fit_path in fit_files:
        m = re.search(r'mc2_(\d+)', fit_path.stem, re.IGNORECASE)
        if not m:
            continue
        mission_id = int(m.group(1))

        warriors = parse_fit_warriors(fit_path)
        for part_id, wd in warriors.items():
            brain_lower = wd['brain'].lower()
            if brain_lower not in brains:
                continue

            key = (brain_lower, mission_id)
            if key not in chains:
                brain_data = brains[brain_lower]
                chains[key] = {
                    'brain_name':  brain_data['brain_name'],
                    'mission_id':  mission_id,
                    'patrol_type': brain_data['patrol_type'],
                    'num_cycles':  brain_data['num_cycles'],
                    'team_id':     wd['team_id'],
                    'units':       [],
                    'waypoints':   brain_data['waypoints'],
                }

            chains[key]['units'].append({
                'part_id': part_id,
                'spawn':   [wd['pos_x'], wd['pos_y'], 0.0],
            })

    chain_list = sorted(chains.values(), key=lambda c: (c['mission_id'], c['brain_name']))

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, 'w') as f:
        json.dump({'chains': chain_list}, f, indent=2)

    print(f'[extract_patrol] Written {len(chain_list)} patrol chains to {output_path}')

    # Summary
    by_mission: dict[int, int] = defaultdict(int)
    for c in chain_list:
        by_mission[c['mission_id']] += 1
    for mid in sorted(by_mission):
        print(f'  Mission {mid:02d}: {by_mission[mid]} patrol chains')


if __name__ == '__main__':
    main()
