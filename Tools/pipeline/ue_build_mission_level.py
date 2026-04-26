"""
ue_build_mission_level.py — run inside Unreal Editor Python console
Creates one UE level per MC2 mission using JSON sidecars from extract_mission_layout.py.
Handles P5.4.1, P5.4.2, P5.4.4, P5.4.6, and P5.4.7.

Prerequisites:
    1. Run extract_mission_layout.py offline to produce /tmp/mc2_mission_layouts/mc2_NN.json
    2. Copy Source/Data/terrain/*.tga to HEIGHTMAP_SOURCE_DIR
    3. Run via Tools > Execute Python Script with the UE project open

Config: set MISSION_ID to build one level, or set to None to build all 24.
"""

import unreal
import json
from pathlib import Path

# ── Config ────────────────────────────────────────────────────────────────────
LAYOUT_DIR         = r"/tmp/mc2_mission_layouts"
HEIGHTMAP_SOURCE   = r"/Volumes/projects/personal/MechCommander2-Source/Source/Data/terrain"
LEVELS_PACKAGE     = "/Game/Maps"
MISSION_SCRIPT_BP  = "/Game/Blueprints/Mission/BP_MC2MissionScript"
TRIGGER_VOLUME_BP  = "/Game/Blueprints/Mission/BP_MC2MissionVolume"

# Set to a single integer to build one level, or None to build all
MISSION_ID: int | None = None

# ── Biome → sky/lighting settings ────────────────────────────────────────────
BIOME_PRESETS = {
    "temperate_clear":    {"sun_intensity": 10.0, "sun_color": [1.0, 0.97, 0.88], "fog_density": 0.005,  "sky_tint": [0.28, 0.55, 1.0]},
    "temperate_overcast": {"sun_intensity":  3.0, "sun_color": [0.8, 0.85, 0.9],  "fog_density": 0.02,   "sky_tint": [0.5,  0.6,  0.7]},
    "temperate_storm":    {"sun_intensity":  1.0, "sun_color": [0.5, 0.5,  0.55], "fog_density": 0.04,   "sky_tint": [0.3,  0.35, 0.4]},
    "temperate_dusk":     {"sun_intensity":  4.0, "sun_color": [1.0, 0.5,  0.2],  "fog_density": 0.015,  "sky_tint": [0.9,  0.5,  0.2]},
    "arctic_clear":       {"sun_intensity":  8.0, "sun_color": [0.9, 0.95, 1.0],  "fog_density": 0.003,  "sky_tint": [0.15, 0.35, 0.8]},
    "arctic_night":       {"sun_intensity":  0.5, "sun_color": [0.3, 0.3,  0.5],  "fog_density": 0.01,   "sky_tint": [0.05, 0.05, 0.2]},
    "desert_clear":       {"sun_intensity": 12.0, "sun_color": [1.0, 0.9,  0.7],  "fog_density": 0.008,  "sky_tint": [0.5,  0.65, 0.9]},
}


# ── Helpers ───────────────────────────────────────────────────────────────────

def ue_name(mission_id: int, mission_name: str) -> str:
    safe = ''.join(c if c.isalnum() or c == '_' else '_' for c in mission_name)
    safe = safe.strip('_')[:30]
    return f"L_MC2_{mission_id:02d}_{safe}"


def mc2_to_ue_xy(x_m: float, y_m: float) -> unreal.Vector:
    return unreal.Vector(x_m * 100.0, -y_m * 100.0, 0.0)


def create_level(level_package: str) -> bool:
    """Create a new empty level. Returns True if created, False if already exists."""
    if unreal.EditorAssetLibrary.does_asset_exist(level_package):
        print(f'[MC2Level] Level already exists, skipping create: {level_package}')
        return False
    unreal.EditorLevelLibrary.new_level(level_package)
    return True


def import_heightmap(heightmap_tga: str, mission_name: str, extent_m: int,
                     h_min: int, h_max: int) -> bool:
    """
    Import the TGA heightmap as a UE Landscape.
    extent_m: full map width/height in metres.
    UE Landscape Z range is driven by h_min/h_max (in metres → cm).
    """
    src = Path(HEIGHTMAP_SOURCE) / heightmap_tga
    if not src.exists():
        unreal.log_warning(f'[MC2Level] Heightmap not found: {src}')
        return False

    # Determine landscape quads from TGA resolution
    # (TGA filename tells us e.g. 80x80 → 80 verts per side → 79 quads per side)
    stem = Path(heightmap_tga).stem  # e.g. "80x80"
    parts = stem.split('x')
    if len(parts) == 2:
        try:
            verts_x = int(parts[0])
            verts_y = int(parts[1])
        except ValueError:
            verts_x = verts_y = 64
    else:
        verts_x = verts_y = 64

    # UE wants quads: verts - 1, but must be (2^n × section_size) aligned.
    # Nearest valid UE Landscape size ≥ (verts-1):
    quads = verts_x - 1   # 79 for 80×80, etc.
    # Scale: extent_m metres / quads quads → cm per quad
    scale_xy = (extent_m * 100.0) / quads
    scale_z  = (h_max - h_min) * 100.0 / 512.0   # UE maps 0-65535 to -256..256 in Z units

    landscape_settings = unreal.LandscapeImportLayerInfo()

    # Use SubsystemHelper to create landscape from heightmap
    factory = unreal.LandscapeEditorObject()
    # The simplest scriptable path: EditorScriptingTerrainImport (if plugin available)
    # Otherwise fall back to creating a flat landscape and noting the manual step
    try:
        editor_filter = unreal.AssetImportTask()
        editor_filter.filename      = str(src)
        editor_filter.destination_path = f'/Game/Textures/Terrain'
        editor_filter.automated     = True
        editor_filter.save          = False
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([editor_filter])

        # Note: full Landscape API requires EditorScripting plugin's
        # LandscapeEditorUtilityLibrary which isn't always available.
        # Print instructions for manual import instead.
        print(f'[MC2Level] Heightmap imported as texture: /Game/Textures/Terrain/{Path(heightmap_tga).stem}')
        print(f'[MC2Level] Manual step: Landscape > Import from File, select the TGA above.')
        print(f'[MC2Level]   Scale XY: {scale_xy:.1f} cm/quad   Scale Z: {scale_z:.2f}')
        print(f'[MC2Level]   Quads: {quads}×{quads}   Height range: {h_min}-{h_max}m')
        return True
    except Exception as e:
        unreal.log_warning(f'[MC2Level] Heightmap import partial: {e}')
        return False


def setup_lighting(world, biome: str, weather: dict):
    """Configure DirectionalLight, SkyLight, ExponentialHeightFog per biome + weather."""
    preset = BIOME_PRESETS.get(biome, BIOME_PRESETS["temperate_clear"])

    # Find or spawn ExponentialHeightFog
    fog_actors = unreal.GameplayStatics.get_all_actors_of_class(
        world, unreal.ExponentialHeightFog)
    if fog_actors:
        fog = fog_actors[0]
    else:
        fog = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.ExponentialHeightFog,
            unreal.Vector(0, 0, 200),
            unreal.Rotator()
        )

    if fog:
        fog_comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
        if fog_comp:
            # Fog color from weather data
            fog_rgba = weather.get('fog_color_rgba', [200, 200, 200, 0])
            fog_color = unreal.LinearColor(
                fog_rgba[0] / 255.0,
                fog_rgba[1] / 255.0,
                fog_rgba[2] / 255.0,
                1.0
            )
            fog_density = max(weather.get('fog_density', 0.0), preset['fog_density'])
            fog_comp.set_editor_property('fog_inscattering_color', fog_color)
            fog_comp.set_editor_property('fog_density', fog_density)

    # Find DirectionalLight (sun) and set intensity + color
    sun_actors = unreal.GameplayStatics.get_all_actors_of_class(
        world, unreal.DirectionalLight)
    if sun_actors:
        sun = sun_actors[0]
        sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
        if sun_comp:
            sc = preset['sun_color']
            sun_comp.set_editor_property('intensity', preset['sun_intensity'])
            sun_comp.set_editor_property('light_color',
                unreal.Color(int(sc[0]*255), int(sc[1]*255), int(sc[2]*255), 255))
            # Night missions: tilt sun low
            if 'night' in biome:
                sun.set_actor_rotation(unreal.Rotator(-10, 45, 0), False)
            else:
                sun.set_actor_rotation(unreal.Rotator(-45, 45, 0), False)


def place_trigger_volumes(world, trigger_areas: list, bp_class):
    """Spawn AMC2MissionVolume actors for each trigger area."""
    for area in trigger_areas:
        pos = mc2_to_ue_xy(area['center_x_m'], area['center_y_m'])

        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            bp_class, pos, unreal.Rotator())
        if not actor:
            continue

        actor.set_actor_label(f'TriggerVol_{area["name"]}')

        # Scale the box extents to match the MC2 area size
        half_w_cm = area['half_w_m'] * 100.0
        half_h_cm = area['half_h_m'] * 100.0
        actor.set_actor_scale3d(unreal.Vector(
            half_w_cm / 100.0,   # Box component extent is 100 UU by default
            half_h_cm / 100.0,
            5.0                  # 500 UU tall — enough for any mech
        ))

        try:
            actor.set_editor_property('AreaID', unreal.Name(area['name']))
        except Exception:
            pass


def place_mission_script(world, abl_script: str, mission_script_class):
    """Spawn BP_MC2MissionScript actor with the ABL script name set."""
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        mission_script_class,
        unreal.Vector(0, 0, 100),
        unreal.Rotator()
    )
    if actor:
        actor.set_actor_label(f'MissionScript_{abl_script}')
        try:
            actor.set_editor_property('ABLScriptName', abl_script)
        except Exception:
            pass


# ── Main ─────────────────────────────────────────────────────────────────────

def build_mission(layout: dict):
    mission_id   = layout['mission_id']
    mission_name = layout['name']
    terrain      = layout['terrain']
    weather      = layout['weather']
    triggers     = layout['trigger_areas']
    abl_script   = layout['abl_script']

    level_name    = ue_name(mission_id, mission_name)
    level_package = f'{LEVELS_PACKAGE}/{level_name}'

    print(f'\n[MC2Level] Building: {level_name}')

    # P5.4.1 — Create the level
    create_level(level_package)
    unreal.EditorLevelLibrary.load_level(level_package)
    world = unreal.EditorLevelLibrary.get_editor_world()

    # P5.4.2 — Import/register heightmap, print manual Landscape instructions
    import_heightmap(
        terrain['heightmap'], mission_name,
        terrain['extent_m'], terrain['height_min_m'], terrain['height_max_m']
    )

    # P5.4.4 — Trigger volumes
    trigger_bp = unreal.EditorAssetLibrary.load_blueprint_class(TRIGGER_VOLUME_BP)
    if triggers and trigger_bp:
        place_trigger_volumes(world, triggers, trigger_bp)
        print(f'[MC2Level]   Placed {len(triggers)} trigger volumes')

    # P5.4.6 — Mission script actor
    script_bp = unreal.EditorAssetLibrary.load_blueprint_class(MISSION_SCRIPT_BP)
    if script_bp:
        place_mission_script(world, abl_script, script_bp)

    # P5.4.7 — Sky / lighting
    setup_lighting(world, terrain['biome'], weather)
    if weather['has_rain']:
        print(f'[MC2Level]   Rain system: spawn BP_MC2Rain Niagara actor manually '
              f'(drops={weather["rain_drops"]}, lightning={weather["lightning_chance"]:.2f})')

    unreal.EditorLevelLibrary.save_current_level()
    print(f'[MC2Level]   Saved: {level_package}')


def run():
    layout_dir = Path(LAYOUT_DIR)
    if not layout_dir.exists():
        unreal.log_error(f'[MC2Level] Layout dir not found: {LAYOUT_DIR}')
        return

    if MISSION_ID is not None:
        files = [layout_dir / f'mc2_{MISSION_ID:02d}.json']
    else:
        files = sorted(layout_dir.glob('mc2_[0-9]*.json'))

    files = [f for f in files if f.exists()]
    print(f'[MC2Level] Building {len(files)} mission level(s)')

    with unreal.ScopedSlowTask(len(files), 'Building MC2 Mission Levels') as task:
        task.make_dialog(True)
        for json_path in files:
            if task.should_cancel():
                break
            layout = json.loads(json_path.read_text())
            task.enter_progress_frame(1, f'mc2_{layout["mission_id"]:02d}')
            build_mission(layout)

    print('\n[MC2Level] All levels built.')
    print('[MC2Level] Remaining manual steps per level:')
    print('[MC2Level]   1. Landscape > Import from File (see printed scale values)')
    print('[MC2Level]   2. Paint Landscape material layers per biome')
    print('[MC2Level]   3. Place streaming sublevel references (P5.4 streaming)')
    print('[MC2Level]   4. Add BP_MC2Rain actor to rain missions (mc2_10)')


run()
