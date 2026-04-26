"""
ue_place_waypoints.py — run inside Unreal Editor Python console
Places AMC2WaypointActor chains for one mission level using the JSON produced
by extract_patrol_paths.py.

Prerequisites:
    1. Run extract_patrol_paths.py offline to produce mc2_patrol_paths.json
    2. Open the target mission level (L_MC2_NN_*) in the UE editor
    3. Set PATROL_JSON and MISSION_ID below
    4. Run via Tools > Execute Python Script

What it does:
    - For each patrol chain assigned to this mission, spawns a series of
      AMC2WaypointActor instances and wires them via the NextWaypoint property.
    - Sets bPingPong = True for PATROL_TYPE_LINEAR (ping-pong traversal).
    - Sets bLooping  = True for PATROL_TYPE_LOOPING (circular loop).
    - Labels each actor "WP_<brain>_<point_index>" for easy identification.
    - Groups all waypoints for a chain under a folder "Waypoints/<brain_name>".

Coordinate conversion (MC2 → UE):
    UE_X =  mc2_x * 100   (metres → cm)
    UE_Y = -mc2_y * 100   (flip Y axis)
    UE_Z = landscape height at (UE_X, UE_Y), queried via LineTrace
"""

import unreal
import json
from pathlib import Path

PATROL_JSON = r"/tmp/mc2_patrol_paths.json"
MISSION_ID  = 18   # change to match the level you have open

WAYPOINT_BP = "/Game/Blueprints/Mission/BP_MC2WaypointActor"

# Height above terrain to start the Z line trace from
TRACE_START_Z = 50000.0   # 500m above


def mc2_to_ue(x: float, y: float) -> unreal.Vector:
    return unreal.Vector(x * 100.0, -y * 100.0, 0.0)


def query_landscape_z(world, ue_pos: unreal.Vector) -> float:
    """Line trace downward to find landscape Z at this XY."""
    start = unreal.Vector(ue_pos.x, ue_pos.y, TRACE_START_Z)
    end   = unreal.Vector(ue_pos.x, ue_pos.y, -10000.0)
    hit_result, did_hit = unreal.SystemLibrary.line_trace_single(
        world, start, end,
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,   # WorldStatic channel
        False, [], unreal.DrawDebugTrace.NONE, True, unreal.LinearColor.RED,
        unreal.LinearColor.GREEN, 5.0
    )
    if did_hit:
        return hit_result.impact_point.z + 10.0   # 10cm above ground
    return 0.0


def place_chain(world, chain: dict, bp_class) -> list:
    """
    Spawn all waypoint actors for one patrol chain.
    Returns list of placed actors in waypoint order.
    """
    waypoints  = chain['waypoints']
    brain_name = chain['brain_name']
    ping_pong  = chain['patrol_type'] == 'linear'
    looping    = chain['patrol_type'] == 'looping'

    placed_actors = []

    for i, (wx, wy) in enumerate(waypoints):
        ue_pos = mc2_to_ue(wx, wy)
        ue_pos.z = query_landscape_z(world, ue_pos)

        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            bp_class,
            ue_pos,
            unreal.Rotator(0, 0, 0)
        )
        if not actor:
            unreal.log_warning(f'[MC2WP] Failed to spawn waypoint {i} for {brain_name}')
            continue

        label = f'WP_{brain_name}_{i:02d}'
        actor.set_actor_label(label)

        # Set patrol type flags
        try:
            actor.set_editor_property('bPingPong', ping_pong)
            actor.set_editor_property('bLooping',  looping)
        except Exception:
            pass  # property names may vary from Blueprint defaults

        placed_actors.append(actor)

    # Wire NextWaypoint chain: each actor → next, last → first if looping
    for i, actor in enumerate(placed_actors):
        if i + 1 < len(placed_actors):
            next_actor = placed_actors[i + 1]
        elif looping and placed_actors:
            next_actor = placed_actors[0]   # circular loop back to start
        else:
            next_actor = None   # linear: stop at end (ping-pong handled in BTTask)

        if next_actor:
            try:
                actor.set_editor_property('NextWaypoint', next_actor)
            except Exception:
                pass

    return placed_actors


def run():
    json_path = Path(PATROL_JSON)
    if not json_path.exists():
        unreal.log_error(f'[MC2WP] Patrol JSON not found: {PATROL_JSON}')
        return

    with open(json_path) as f:
        data = json.load(f)

    # Filter to this mission
    mission_chains = [c for c in data.get('chains', []) if c['mission_id'] == MISSION_ID]
    if not mission_chains:
        print(f'[MC2WP] No patrol chains for mission {MISSION_ID}')
        return

    print(f'[MC2WP] Placing {len(mission_chains)} patrol chains for mission {MISSION_ID}')

    bp_class = unreal.EditorAssetLibrary.load_blueprint_class(WAYPOINT_BP)
    if not bp_class:
        unreal.log_error(f'[MC2WP] Blueprint not found: {WAYPOINT_BP}')
        return

    world       = unreal.EditorLevelLibrary.get_editor_world()
    total_pts   = 0
    total_chains = 0

    with unreal.ScopedSlowTask(len(mission_chains), 'Placing Patrol Waypoints') as task:
        task.make_dialog(True)

        for chain in mission_chains:
            if task.should_cancel():
                break
            task.enter_progress_frame(1, chain['brain_name'])

            actors = place_chain(world, chain, bp_class)
            total_pts    += len(actors)
            total_chains += 1

            # Report which Part actors should reference this chain's first waypoint
            if actors:
                first_label = actors[0].get_actor_label()
                units_str   = ', '.join(f"Part{u['part_id']}" for u in chain['units'])
                print(f'[MC2WP]   {chain["brain_name"]}: {len(actors)} pts '
                      f'({chain["patrol_type"]}) | units: {units_str} | start: {first_label}')

    print(f'[MC2WP] Done. {total_chains} chains, {total_pts} waypoint actors placed.')
    print('[MC2WP] Save the level to persist placement.')
    print('[MC2WP] Wire the first waypoint of each chain to its unit actor via')
    print('[MC2WP]   BP_BattleMech.PatrolStartWaypoint in the Details panel.')


run()
