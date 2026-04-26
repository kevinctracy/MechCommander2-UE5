"""
ue_build_sound_cues.py — run inside Unreal Editor Python console
Generates Sound Cues for MC2 audio assets that have multiple variants.

Run AFTER ue_import_audio.py has finished importing all SoundWave assets.

What it does
------------
For each SoundWave group that shares a base name (e.g. Weapon_AC2_Fire_01,
Weapon_AC2_Fire_02, Weapon_AC2_Fire_03), this script:
  1. Creates a SoundCue at /Game/Audio/<Category>/SC_<BaseName>
  2. Adds a SoundNodeRandom mixing all variant waves
  3. Sets weights equally across variants
  4. Applies the category's SoundClass (SC_SFX / SC_VO / SC_Music / SC_UI)
  5. Saves the cue

Single-variant waves get a trivial SoundCue wrapping just the SoundWave
(avoids Blueprint code having to care about wave vs. cue).

Usage:
    Run ue_import_audio.py first, then run this script.
    No configuration required — it scans /Game/Audio/ automatically.
"""

import unreal
import re
from collections import defaultdict

AUDIO_ROOT     = "/Game/Audio"
SOUND_CLASS_MAP = {
    "SFX":   "/Game/Audio/SoundClasses/SC_SFX",
    "VO":    "/Game/Audio/SoundClasses/SC_VO",
    "Music": "/Game/Audio/SoundClasses/SC_Music",
    "UI":    "/Game/Audio/SoundClasses/SC_UI",
}

VARIANT_PATTERN = re.compile(r"^(.+?)_(\d+)$")


def get_base_name(wave_name: str) -> str:
    """Strip trailing _01, _02 … suffix to get the variant group base name."""
    m = VARIANT_PATTERN.match(wave_name)
    return m.group(1) if m else wave_name


def get_sound_class(package_path: str):
    """Return the SoundClass asset for a path like /Game/Audio/SFX/Weapons/…"""
    parts = package_path.replace(AUDIO_ROOT + "/", "").split("/")
    top   = parts[0] if parts else ""
    sc_path = SOUND_CLASS_MAP.get(top)
    if sc_path and unreal.EditorAssetLibrary.does_asset_exist(sc_path):
        return unreal.load_asset(sc_path)
    return None


def collect_waves():
    """Return dict: category_dir → base_name → [SoundWave assets]"""
    registry    = unreal.AssetRegistryHelpers.get_asset_registry()
    ar_filter   = unreal.ARFilter(
        class_names=["SoundWave"],
        package_paths=[AUDIO_ROOT],
        recursive_paths=True
    )
    assets = registry.get_assets(ar_filter)

    groups: dict[str, dict[str, list]] = defaultdict(lambda: defaultdict(list))

    for asset_data in assets:
        pkg     = asset_data.package_path   # e.g. /Game/Audio/SFX/Weapons
        name    = asset_data.asset_name     # e.g. Weapon_AC2_Fire_01
        base    = get_base_name(str(name))
        sw      = unreal.load_asset(f"{pkg}/{name}")
        if sw:
            groups[str(pkg)][base].append(sw)

    return groups


def cue_already_exists(cue_pkg: str) -> bool:
    return unreal.EditorAssetLibrary.does_asset_exist(cue_pkg)


def build_cue(cue_dir: str, base_name: str, waves: list, sound_class):
    """Create a SoundCue asset with a SoundNodeRandom mixing all waves."""
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    cue_name    = f"SC_{base_name}"
    cue_pkg     = f"{cue_dir}/{cue_name}"

    if cue_already_exists(cue_pkg):
        return False   # skip

    cue: unreal.SoundCue = asset_tools.create_asset(
        cue_name, cue_dir,
        unreal.SoundCue, unreal.SoundCueFactoryNew()
    )
    if not cue:
        unreal.log_warning(f"[MC2Cue] Failed to create cue: {cue_pkg}")
        return False

    if sound_class:
        cue.set_editor_property("sound_class_object", sound_class)

    graph: unreal.SoundCueGraph = cue.get_editor_property("sound_cue_graph")

    # Single-variant → SoundNodeWavePlayer directly
    if len(waves) == 1:
        wave_node = unreal.SoundCueGraphNode()
        # Use the factory helper instead
        _wire_single_wave(cue, waves[0])
    else:
        _wire_random_waves(cue, waves)

    unreal.EditorAssetLibrary.save_asset(cue_pkg)
    return True


def _wire_single_wave(cue: unreal.SoundCue, wave: unreal.SoundWave):
    """Set the first (output) node to play a single wave."""
    # The simplest approach: use SoundCue's first_node slot via property
    # (Full graph node manipulation requires EditorGraph access; we use
    # the public API on SoundCue instead.)
    node = unreal.SoundNodeWavePlayer()
    node.set_editor_property("sound_wave", wave)
    cue.set_editor_property("first_node", node)


def _wire_random_waves(cue: unreal.SoundCue, waves: list):
    """Wire a SoundNodeRandom → multiple SoundNodeWavePlayer nodes."""
    random_node = unreal.SoundNodeRandom()
    weight      = 1.0 / len(waves)
    weights     = [weight] * len(waves)
    random_node.set_editor_property("weights", weights)
    random_node.set_editor_property("randomize_without_replacement", True)

    child_nodes = []
    for wave in waves:
        wp = unreal.SoundNodeWavePlayer()
        wp.set_editor_property("sound_wave", wave)
        child_nodes.append(wp)

    random_node.set_editor_property("child_nodes", child_nodes)
    cue.set_editor_property("first_node", random_node)


def run():
    groups   = collect_waves()
    total    = sum(len(bases) for bases in groups.values())
    created  = 0
    skipped  = 0

    print(f"[MC2Cue] Found {total} wave groups across {len(groups)} directories.")

    with unreal.ScopedSlowTask(total, "Building Sound Cues") as task:
        task.make_dialog(True)

        for pkg_dir, bases in groups.items():
            sc = get_sound_class(pkg_dir)
            for base_name, waves in bases.items():
                if task.should_cancel():
                    break
                task.enter_progress_frame(1, f"SC_{base_name}")
                ok = build_cue(pkg_dir, base_name, waves, sc)
                if ok:
                    created += 1
                else:
                    skipped += 1

    print(f"[MC2Cue] Done. Created: {created}  Skipped (exist): {skipped}")


run()
