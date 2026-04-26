"""
ue_build_mission.py — run inside Unreal Editor Python console
Full mission builder: creates level, imports heightmap, places all actors,
wires patrol waypoints, sets up trigger volumes, mission script, and lighting.

Replaces: ue_build_mission_level.py, ue_place_mission_actors.py, ue_place_waypoints.py

Prerequisites:
    1. Run extract_all_mission_data.py offline to produce /tmp/mc2_missions/mc2_NN.json
    2. Copy Source/Data/terrain/*.tga to HEIGHTMAP_SOURCE_DIR
    3. Run via Tools > Execute Python Script with the UE project open

Config: set MISSION_ID to build one level, or None to build all 24.
"""

import unreal
import json
from pathlib import Path

# ── Config ────────────────────────────────────────────────────────────────────
MISSION_DATA_DIR  = r"/tmp/mc2_missions"
HEIGHTMAP_SOURCE  = r"/Volumes/projects/personal/MechCommander2-Source/Source/Data/terrain"
LEVELS_PACKAGE    = "/Game/Maps"
BLUEPRINTS_ROOT   = "/Game/Blueprints"

MISSION_SCRIPT_BP  = f"{BLUEPRINTS_ROOT}/Mission/BP_MC2MissionScript"
TRIGGER_VOLUME_BP  = f"{BLUEPRINTS_ROOT}/Mission/BP_MC2MissionVolume"
WAYPOINT_BP        = f"{BLUEPRINTS_ROOT}/Mission/BP_MC2WaypointActor"
OBJECTIVE_BP       = f"{BLUEPRINTS_ROOT}/Mission/BP_MC2Objective"

# Kind → Blueprint path (used for actor placement)
KIND_TO_BP = {
    "mech":      f"{BLUEPRINTS_ROOT}/Units/BP_BattleMech",
    "vehicle":   f"{BLUEPRINTS_ROOT}/Units/BP_GroundVehicle",
    "building":  f"{BLUEPRINTS_ROOT}/World/BP_Building",
    "turret":    f"{BLUEPRINTS_ROOT}/World/BP_Turret",
    "dropship":  f"{BLUEPRINTS_ROOT}/Units/BP_Dropship",
    "prop":      f"{BLUEPRINTS_ROOT}/World/BP_TerrainProp",
}

# Set to a single integer to build one level, or None to build all
MISSION_ID: int | None = None

# Height above terrain for waypoint Z line trace origin
TRACE_START_Z = 50000.0

# ── Biome → sky/lighting settings ────────────────────────────────────────────
BIOME_PRESETS = {
    "temperate_clear":    {"sun_intensity": 10.0, "sun_color": [1.0, 0.97, 0.88], "fog_density": 0.005},
    "temperate_overcast": {"sun_intensity":  3.0, "sun_color": [0.8, 0.85, 0.9],  "fog_density": 0.02},
    "temperate_storm":    {"sun_intensity":  1.0, "sun_color": [0.5, 0.5,  0.55], "fog_density": 0.04},
    "temperate_dusk":     {"sun_intensity":  4.0, "sun_color": [1.0, 0.5,  0.2],  "fog_density": 0.015},
    "arctic_clear":       {"sun_intensity":  8.0, "sun_color": [0.9, 0.95, 1.0],  "fog_density": 0.003},
    "arctic_night":       {"sun_intensity":  0.5, "sun_color": [0.3, 0.3,  0.5],  "fog_density": 0.01},
    "desert_clear":       {"sun_intensity": 12.0, "sun_color": [1.0, 0.9,  0.7],  "fog_density": 0.008},
}


# ── Coordinate conversion ─────────────────────────────────────────────────────

def mc2_to_ue_xy(x_m: float, y_m: float, z_cm: float = 0.0) -> unreal.Vector:
    """MC2 world metres (X, Y) → UE cm (X, -Y, Z)."""
    return unreal.Vector(x_m * 100.0, -y_m * 100.0, z_cm)


# ── Level creation ────────────────────────────────────────────────────────────

def level_name(mission_id: int, name: str) -> str:
    safe = ''.join(c if c.isalnum() or c == '_' else '_' for c in name).strip('_')[:30]
    return f"L_MC2_{mission_id:02d}_{safe}"


def create_and_load_level(package: str):
    if unreal.EditorAssetLibrary.does_asset_exist(package):
        print(f'[MC2Build] Level exists, loading: {package}')
    else:
        unreal.EditorLevelLibrary.new_level(package)
    unreal.EditorLevelLibrary.load_level(package)


# ── Heightmap import ──────────────────────────────────────────────────────────

def import_heightmap(terrain: dict):
    src = Path(HEIGHTMAP_SOURCE) / terrain['heightmap']
    if not src.exists():
        unreal.log_warning(f'[MC2Build] Heightmap not found: {src}')
        return

    stem  = Path(terrain['heightmap']).stem  # "80x80"
    parts = stem.split('x')
    try:
        verts = int(parts[0]) if len(parts) == 2 else 64
    except ValueError:
        verts = 64

    quads    = verts - 1
    extent_m = terrain['extent_m']
    scale_xy = (extent_m * 100.0) / quads
    scale_z  = (terrain['height_max_m'] - terrain['height_min_m']) * 100.0 / 512.0

    task = unreal.AssetImportTask()
    task.filename         = str(src)
    task.destination_path = '/Game/Textures/Terrain'
    task.automated        = True
    task.save             = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    print(f'[MC2Build] Heightmap → /Game/Textures/Terrain/{stem}')
    print(f'[MC2Build]   Manual: Landscape > Import from File, scale XY={scale_xy:.1f} Z={scale_z:.2f}, quads={quads}')


# ── Lighting setup ────────────────────────────────────────────────────────────

def setup_lighting(world, biome: str, weather: dict):
    preset = BIOME_PRESETS.get(biome, BIOME_PRESETS['temperate_clear'])

    # ExponentialHeightFog
    fog_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ExponentialHeightFog)
    fog = fog_actors[0] if fog_actors else unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.ExponentialHeightFog, unreal.Vector(0, 0, 200), unreal.Rotator())
    if fog:
        fog_comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
        if fog_comp:
            rgba = weather.get('fog_color_rgba', [200, 200, 200, 0])
            fog_comp.set_editor_property('fog_inscattering_color',
                unreal.LinearColor(rgba[0] / 255.0, rgba[1] / 255.0, rgba[2] / 255.0, 1.0))
            fog_comp.set_editor_property('fog_density',
                max(weather.get('fog_density', 0.0), preset['fog_density']))

    # DirectionalLight
    sun_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.DirectionalLight)
    if sun_actors:
        sun      = sun_actors[0]
        sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
        if sun_comp:
            sc = preset['sun_color']
            sun_comp.set_editor_property('intensity', preset['sun_intensity'])
            sun_comp.set_editor_property('light_color',
                unreal.Color(int(sc[0] * 255), int(sc[1] * 255), int(sc[2] * 255), 255))
            pitch = -10 if 'night' in biome else -45
            sun.set_actor_rotation(unreal.Rotator(pitch, 45, 0), False)


# ── Unit actor placement ──────────────────────────────────────────────────────

def place_actors(world, actors: list, bp_cache: dict) -> dict[int, unreal.Actor]:
    """Spawn all Part actors. Returns {part_id: actor}."""
    placed: dict[int, unreal.Actor] = {}
    skipped = 0

    for a in actors:
        kind   = a['kind']
        bp_path = KIND_TO_BP.get(kind)
        if not bp_path:
            skipped += 1
            continue

        if bp_path not in bp_cache:
            bp_cache[bp_path] = unreal.EditorAssetLibrary.load_blueprint_class(bp_path)
        bp_class = bp_cache[bp_path]
        if not bp_class:
            skipped += 1
            continue

        pos = mc2_to_ue_xy(a['position_m'][0], a['position_m'][1])
        rot = unreal.Rotator(0.0, a['rotation_deg'], 0.0)
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(bp_class, pos, rot)
        if not actor:
            skipped += 1
            continue

        label = f"{kind.capitalize()}_{a['part_id']:03d}_{a['csv_file']}"
        actor.set_actor_label(label)

        try:
            actor.set_editor_property('TeamIndex', a['team_id'])
        except Exception:
            pass
        try:
            actor.set_editor_property('CSVType', a['csv_file'])
        except Exception:
            pass
        if a['brain']:
            try:
                actor.set_editor_property('PatrolBrainName', a['brain'])
            except Exception:
                pass

        placed[a['part_id']] = actor

    print(f'[MC2Build]   Actors: {len(placed)} placed, {skipped} skipped (missing BP)')
    return placed


# ── Trigger volumes ───────────────────────────────────────────────────────────

def place_trigger_volumes(world, trigger_areas: list, bp_class):
    for area in trigger_areas:
        pos   = mc2_to_ue_xy(area['center_x_m'], area['center_y_m'])
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(bp_class, pos, unreal.Rotator())
        if not actor:
            continue
        actor.set_actor_label(f'TriggerVol_{area["name"]}')
        actor.set_actor_scale3d(unreal.Vector(
            area['half_w_m'],   # 1 UU = 1cm; default box = 100cm → scale = metres directly
            area['half_h_m'],
            5.0                 # 500cm tall
        ))
        try:
            actor.set_editor_property('AreaID', unreal.Name(area['name']))
        except Exception:
            pass


# ── Objectives ────────────────────────────────────────────────────────────────

def place_objectives(world, objectives: list, bp_cache: dict):
    bp_path = OBJECTIVE_BP
    if bp_path not in bp_cache:
        bp_cache[bp_path] = unreal.EditorAssetLibrary.load_blueprint_class(bp_path)
    bp_class = bp_cache[bp_path]
    if not bp_class:
        return

    for obj in objectives:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            bp_class, unreal.Vector(0, 0, 100 + obj['id'] * 50), unreal.Rotator())
        if not actor:
            continue
        actor.set_actor_label(f'Objective_{obj["id"]}')
        try:
            actor.set_editor_property('ObjectiveText',
                unreal.Text.from_string(obj.get('text', '')))
            actor.set_editor_property('bRequired', obj.get('required', True))
        except Exception:
            pass


# ── Waypoint placement ────────────────────────────────────────────────────────

def query_landscape_z(world, x: float, y: float) -> float:
    start = unreal.Vector(x, y, TRACE_START_Z)
    end   = unreal.Vector(x, y, -10000.0)
    hit, did_hit = unreal.SystemLibrary.line_trace_single(
        world, start, end,
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
        False, [], unreal.DrawDebugTrace.NONE, True,
        unreal.LinearColor.RED, unreal.LinearColor.GREEN, 5.0
    )
    return hit.impact_point.z + 10.0 if did_hit else 0.0


def place_patrol_chains(world, chains: list, bp_cache: dict):
    bp_path = WAYPOINT_BP
    if bp_path not in bp_cache:
        bp_cache[bp_path] = unreal.EditorAssetLibrary.load_blueprint_class(bp_path)
    bp_class = bp_cache[bp_path]
    if not bp_class:
        unreal.log_warning(f'[MC2Build] Waypoint BP not found: {WAYPOINT_BP}')
        return

    total_pts = 0
    for chain in chains:
        brain_name = chain['brain_name']
        looping    = chain['patrol_type'] == 'looping'
        ping_pong  = chain['patrol_type'] == 'linear'
        placed_wp  = []

        for i, (wx, wy) in enumerate(chain['waypoints']):
            ue_x = wx * 100.0
            ue_y = -wy * 100.0
            ue_z = query_landscape_z(world, ue_x, ue_y)

            actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
                bp_class, unreal.Vector(ue_x, ue_y, ue_z), unreal.Rotator())
            if not actor:
                continue
            actor.set_actor_label(f'WP_{brain_name}_{i:02d}')
            try:
                actor.set_editor_property('bPingPong', ping_pong)
                actor.set_editor_property('bLooping',  looping)
            except Exception:
                pass
            placed_wp.append(actor)

        # Wire NextWaypoint chain
        for i, actor in enumerate(placed_wp):
            if i + 1 < len(placed_wp):
                nxt = placed_wp[i + 1]
            elif looping and placed_wp:
                nxt = placed_wp[0]
            else:
                nxt = None
            if nxt:
                try:
                    actor.set_editor_property('NextWaypoint', nxt)
                except Exception:
                    pass

        total_pts += len(placed_wp)
        units_str = ', '.join(f"Part{u['part_id']}" for u in chain['units'])
        if placed_wp:
            print(f'[MC2Build]   Chain {brain_name}: {len(placed_wp)} pts '
                  f'({chain["patrol_type"]}) units=[{units_str}] start={placed_wp[0].get_actor_label()}')

    print(f'[MC2Build]   Waypoints: {total_pts} total across {len(chains)} chains')


# ── Mission script actor ──────────────────────────────────────────────────────

def place_mission_script(world, abl_script: str, bp_cache: dict):
    bp_path = MISSION_SCRIPT_BP
    if bp_path not in bp_cache:
        bp_cache[bp_path] = unreal.EditorAssetLibrary.load_blueprint_class(bp_path)
    bp_class = bp_cache[bp_path]
    if not bp_class:
        return
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        bp_class, unreal.Vector(0, 0, 100), unreal.Rotator())
    if actor:
        actor.set_actor_label(f'MissionScript_{abl_script}')
        try:
            actor.set_editor_property('ABLScriptName', abl_script)
        except Exception:
            pass


# ── Main per-mission builder ──────────────────────────────────────────────────

def build_mission(layout: dict):
    mission_id = layout['mission_id']
    name       = layout['name']
    terrain    = layout['terrain']
    weather    = layout['weather']

    pkg = f"{LEVELS_PACKAGE}/{level_name(mission_id, name)}"
    print(f'\n[MC2Build] ── Mission {mission_id:02d}: {name} → {pkg}')

    create_and_load_level(pkg)
    world = unreal.EditorLevelLibrary.get_editor_world()

    # Heightmap
    import_heightmap(terrain)

    bp_cache: dict = {}

    # Unit actors (mechs, vehicles, buildings, props)
    actors = layout.get('actors', [])
    if actors:
        place_actors(world, actors, bp_cache)

    # Trigger volumes
    trigger_bp_path = TRIGGER_VOLUME_BP
    if trigger_bp_path not in bp_cache:
        bp_cache[trigger_bp_path] = unreal.EditorAssetLibrary.load_blueprint_class(trigger_bp_path)
    trigger_bp = bp_cache[trigger_bp_path]
    trigger_areas = layout.get('trigger_areas', [])
    if trigger_areas and trigger_bp:
        place_trigger_volumes(world, trigger_areas, trigger_bp)
        print(f'[MC2Build]   Trigger volumes: {len(trigger_areas)}')

    # Objectives
    objectives = layout.get('objectives', [])
    if objectives:
        place_objectives(world, objectives, bp_cache)
        print(f'[MC2Build]   Objectives: {len(objectives)}')

    # Patrol waypoints
    patrol_chains = layout.get('patrol_chains', [])
    if patrol_chains:
        place_patrol_chains(world, patrol_chains, bp_cache)

    # Mission script actor
    place_mission_script(world, layout['abl_script'], bp_cache)

    # Lighting
    setup_lighting(world, terrain['biome'], weather)
    if weather.get('has_rain'):
        print(f'[MC2Build]   Rain: spawn BP_MC2Rain manually '
              f'(drops={weather["rain_drops"]}, lightning={weather["lightning_chance"]:.2f})')

    unreal.EditorLevelLibrary.save_current_level()
    print(f'[MC2Build]   Saved: {pkg}')


# ── Entry point ───────────────────────────────────────────────────────────────

def run():
    data_dir = Path(MISSION_DATA_DIR)
    if not data_dir.exists():
        unreal.log_error(f'[MC2Build] Data dir not found: {MISSION_DATA_DIR}')
        return

    if MISSION_ID is not None:
        files = [data_dir / f'mc2_{MISSION_ID:02d}.json']
    else:
        files = sorted(data_dir.glob('mc2_[0-9]*.json'))
    files = [f for f in files if f.exists()]

    print(f'[MC2Build] Building {len(files)} mission level(s)')
    with unreal.ScopedSlowTask(len(files), 'Building MC2 Mission Levels') as task:
        task.make_dialog(True)
        for json_path in files:
            if task.should_cancel():
                break
            layout = json.loads(json_path.read_text())
            task.enter_progress_frame(1, f'mc2_{layout["mission_id"]:02d}')
            build_mission(layout)

    print('\n[MC2Build] All levels built.')
    print('[MC2Build] Remaining manual steps per level:')
    print('[MC2Build]   1. Landscape > Import from File (see printed scale values above)')
    print('[MC2Build]   2. Paint Landscape material layers per biome')
    print('[MC2Build]   3. Wire PatrolStartWaypoint on mech actors to chain first waypoint')
    print('[MC2Build]   4. Add BP_MC2Rain Niagara actor to rain missions')


run()
