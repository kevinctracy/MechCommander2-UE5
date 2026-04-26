"""
ue_place_mission_actors.py — run inside Unreal Editor Python console
Places MC2 mission actors (mechs, buildings, turrets, objectives, trigger volumes)
into the current level using data extracted from MC2's FIT/ODF/ABL mission files.

Prerequisites
-------------
1. Run extract_mission_data.py (offline tool) to generate a JSON sidecar for each
   mission, e.g. MC2_Mission_01.json.  Schema documented below.
2. Open the target level in the UE editor.
3. Set MISSION_JSON to point at the sidecar file.
4. Run via Tools > Execute Python Script.

Mission JSON schema
-------------------
{
  "mission_id":  1,
  "name":        "Ghost Bear's Lair",
  "actors": [
    {
      "type":        "mech",            // mech | building | turret | objective | trigger
      "blueprint":   "BP_BattleMech",   // asset name under /Game/Blueprints/
      "team":        1,
      "position":    [x, y, z],         // MC2 world coords (converted to UE cm)
      "rotation_yaw": 180.0,
      "variant":     "Mad_Cat",         // mech chassis ID or building type
      "label":       "OPFOR_01"         // optional actor label
    }
  ],
  "objectives": [
    {
      "id":       1,
      "text":     "Destroy the Power Plant",
      "required": true,
      "trigger_actor": "OPFOR_POWERPLANT"
    }
  ]
}

Coordinate mapping: MC2 uses Y-up metres; UE uses Z-up centimetres.
  UE_X =  MC2_X * 100
  UE_Y = -MC2_Z * 100   (MC2 Z → UE Y, flipped)
  UE_Z =  MC2_Y * 100
"""

import unreal
import json
from pathlib import Path

MISSION_JSON    = r"/tmp/mc2_missions/MC2_Mission_01.json"
BLUEPRINT_ROOT  = "/Game/Blueprints"
OBJECTIVES_ROOT = "/Game/Blueprints/Mission"

TYPE_TO_PATH = {
    "mech":      f"{BLUEPRINT_ROOT}/Units/BP_BattleMech",
    "building":  f"{BLUEPRINT_ROOT}/World/BP_Building",
    "turret":    f"{BLUEPRINT_ROOT}/World/BP_Turret",
    "trigger":   f"{BLUEPRINT_ROOT}/Mission/BP_MC2MissionVolume",
    "objective": f"{BLUEPRINT_ROOT}/Mission/BP_MC2Objective",
}


def mc2_to_ue(pos: list) -> unreal.Vector:
    return unreal.Vector(
         pos[0] * 100.0,
        -pos[2] * 100.0,
         pos[1] * 100.0,
    )


def place_actor(editor_level_lib, data: dict) -> unreal.Actor | None:
    actor_type  = data.get("type", "building")
    bp_path     = data.get("blueprint_override") or TYPE_TO_PATH.get(actor_type)
    if not bp_path:
        unreal.log_warning(f"[MC2Place] Unknown type: {actor_type}")
        return None

    bp_class = unreal.EditorAssetLibrary.load_blueprint_class(bp_path)
    if not bp_class:
        unreal.log_warning(f"[MC2Place] Blueprint not found: {bp_path}")
        return None

    pos = mc2_to_ue(data.get("position", [0, 0, 0]))
    rot = unreal.Rotator(0.0, data.get("rotation_yaw", 0.0), 0.0)

    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(bp_class, pos, rot)
    if not actor:
        return None

    label = data.get("label")
    if label:
        actor.set_actor_label(label)

    # Set team index if the actor has a TeamIndex property
    team = data.get("team")
    if team is not None:
        try:
            actor.set_editor_property("TeamIndex", team)
        except Exception:
            pass

    # Set mech chassis if applicable
    variant = data.get("variant")
    if variant:
        try:
            actor.set_editor_property("ChassisID", variant)
        except Exception:
            pass

    return actor


def wire_objectives(level_actors: dict, objectives: list):
    """Find MC2Objective actors and fill in display text + required flag."""
    for obj_data in objectives:
        label = f"Objective_{obj_data.get('id', 0)}"
        obj_actor = unreal.EditorLevelLibrary.get_actor_reference(label)
        if not obj_actor:
            # Spawn a fresh one
            bp_path  = f"{OBJECTIVES_ROOT}/BP_MC2Objective"
            bp_class = unreal.EditorAssetLibrary.load_blueprint_class(bp_path)
            if bp_class:
                obj_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
                    bp_class, unreal.Vector(0, 0, 0), unreal.Rotator())
                obj_actor.set_actor_label(label)

        if obj_actor:
            try:
                obj_actor.set_editor_property("ObjectiveText",
                    unreal.Text.from_string(obj_data.get("text", "")))
                obj_actor.set_editor_property("bRequired",
                    obj_data.get("required", True))
            except Exception:
                pass


def run():
    json_path = Path(MISSION_JSON)
    if not json_path.exists():
        unreal.log_error(f"[MC2Place] Mission JSON not found: {MISSION_JSON}")
        return

    with open(json_path, "r") as f:
        mission = json.load(f)

    actors_data = mission.get("actors", [])
    print(f"[MC2Place] Placing {len(actors_data)} actors for mission: {mission.get('name')}")

    placed    = 0
    failed    = 0
    actor_map = {}   # label → placed actor

    with unreal.ScopedSlowTask(len(actors_data), "Placing MC2 Mission Actors") as task:
        task.make_dialog(True)

        for data in actors_data:
            if task.should_cancel():
                break
            task.enter_progress_frame(1, data.get("label", data.get("type")))

            actor = place_actor(unreal.EditorLevelLibrary, data)
            if actor:
                placed += 1
                lbl = data.get("label")
                if lbl:
                    actor_map[lbl] = actor
            else:
                failed += 1

    wire_objectives(actor_map, mission.get("objectives", []))

    print(f"[MC2Place] Done. Placed: {placed}  Failed: {failed}")
    print("[MC2Place] Save the level to persist actor placement.")


run()
