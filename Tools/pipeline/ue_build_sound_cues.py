"""
ue_build_sound_cues.py — run inside Unreal Editor Python console
Generates Sound Cues for MC2 audio assets that have multiple variants.

Run AFTER ue_import_audio.py has finished importing all SoundWave assets.

What it does
------------
Two grouping strategies are used (RSP groups take priority):

1. RSP files (pilot1.rsp, betty.rsp, Sound.rsp, etc.)
   Each .rsp file lists WAV filenames that form a random pool.
   The RSP stem name becomes the SoundCue base name.
   e.g. pilot1.rsp → SC_Pilot1 with SoundNodeRandom across all listed waves.

2. Numeric suffix groups (_01, _02, _03 …)
   Any SoundWave not covered by an RSP group is grouped by stripping trailing
   numeric suffixes, matching the standard UE variant-naming convention.

For each group this script:
  1. Creates a SoundCue at /Game/Audio/<Category>/SC_<BaseName>
  2. Adds a SoundNodeRandom mixing all variant waves (equal weights)
  3. Applies the category's SoundClass (SC_SFX / SC_VO / SC_Music / SC_UI)
  4. Saves the cue

Single-variant waves get a trivial SoundCue wrapping just the SoundWave.

Configuration
-------------
Set RSP_DIR to the path of Source/Data/Sound/ on this machine so the script
can read the .rsp files before scanning UE assets.
"""

import unreal
import re
import os
from collections import defaultdict
from pathlib import Path

AUDIO_ROOT = "/Game/Audio"

# Path to the MC2 source sound directory containing *.rsp files.
# Edit this before running if your source checkout is elsewhere.
RSP_DIR = r"/Volumes/projects/personal/MechCommander2-Source/Source/Data/Sound"

SOUND_CLASS_MAP = {
    "SFX":   "/Game/Audio/SoundClasses/SC_SFX",
    "VO":    "/Game/Audio/SoundClasses/SC_VO",
    "Music": "/Game/Audio/SoundClasses/SC_Music",
    "UI":    "/Game/Audio/SoundClasses/SC_UI",
}

VARIANT_PATTERN = re.compile(r"^(.+?)_(\d+)$")


# ---------------------------------------------------------------------------
# RSP parsing
# ---------------------------------------------------------------------------

def _wav_stem(filename: str) -> str:
    """Normalise a WAV path from an RSP entry to a bare stem for UE lookup.

    RSP lines may use backslashes: 'betty\\cmp_log_weight.wav'
    We only need the stem because ue_import_audio.py uses the WAV stem as the
    asset name, preserving directory structure under /Game/Audio/.
    """
    p = Path(filename.replace("\\", "/"))
    return p.stem


def load_rsp_groups(rsp_dir: str) -> dict[str, list[str]]:
    """Read all *.rsp files and return {rsp_stem: [wav_stem, ...]}."""
    groups: dict[str, list[str]] = {}
    rsp_path = Path(rsp_dir)
    if not rsp_path.exists():
        unreal.log_warning(f"[MC2Cue] RSP_DIR not found: {rsp_dir} — skipping RSP groups")
        return groups

    for rsp_file in sorted(rsp_path.glob("*.rsp")):
        wav_stems = []
        for line in rsp_file.read_text(encoding="utf-8", errors="replace").splitlines():
            line = line.strip()
            if line:
                wav_stems.append(_wav_stem(line))
        if wav_stems:
            groups[rsp_file.stem] = wav_stems

    return groups


# ---------------------------------------------------------------------------
# Wave collection
# ---------------------------------------------------------------------------

def get_base_name(wave_name: str) -> str:
    m = VARIANT_PATTERN.match(wave_name)
    return m.group(1) if m else wave_name


def get_sound_class(package_path: str):
    parts = package_path.replace(AUDIO_ROOT + "/", "").split("/")
    top = parts[0] if parts else ""
    sc_path = SOUND_CLASS_MAP.get(top)
    if sc_path and unreal.EditorAssetLibrary.does_asset_exist(sc_path):
        return unreal.load_asset(sc_path)
    return None


def collect_all_waves() -> dict[str, unreal.SoundWave]:
    """Return {stem_lower: SoundWave} for every SoundWave under AUDIO_ROOT."""
    registry  = unreal.AssetRegistryHelpers.get_asset_registry()
    ar_filter = unreal.ARFilter(
        class_names=["SoundWave"],
        package_paths=[AUDIO_ROOT],
        recursive_paths=True,
    )
    stem_map: dict[str, unreal.SoundWave] = {}
    for asset_data in registry.get_assets(ar_filter):
        pkg  = str(asset_data.package_path)
        name = str(asset_data.asset_name)
        sw   = unreal.load_asset(f"{pkg}/{name}")
        if sw:
            stem_map[name.lower()] = (sw, pkg)
    return stem_map


def collect_waves_by_numeric_suffix(
        stem_map: dict, covered_stems: set
) -> dict[str, dict[str, list]]:
    """Group uncovered waves by numeric suffix into pkg_dir → base → [waves]."""
    groups: dict[str, dict[str, list]] = defaultdict(lambda: defaultdict(list))
    for stem_lower, (sw, pkg) in stem_map.items():
        if stem_lower in covered_stems:
            continue
        base = get_base_name(stem_lower)
        groups[pkg][base].append(sw)
    return groups


# ---------------------------------------------------------------------------
# Cue building
# ---------------------------------------------------------------------------

def cue_already_exists(cue_pkg: str) -> bool:
    return unreal.EditorAssetLibrary.does_asset_exist(cue_pkg)


def build_cue(cue_dir: str, base_name: str, waves: list, sound_class) -> bool:
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    cue_name    = f"SC_{base_name}"
    cue_pkg     = f"{cue_dir}/{cue_name}"

    if cue_already_exists(cue_pkg):
        return False

    cue: unreal.SoundCue = asset_tools.create_asset(
        cue_name, cue_dir,
        unreal.SoundCue, unreal.SoundCueFactoryNew(),
    )
    if not cue:
        unreal.log_warning(f"[MC2Cue] Failed to create: {cue_pkg}")
        return False

    if sound_class:
        cue.set_editor_property("sound_class_object", sound_class)

    if len(waves) == 1:
        _wire_single_wave(cue, waves[0])
    else:
        _wire_random_waves(cue, waves)

    unreal.EditorAssetLibrary.save_asset(cue_pkg)
    return True


def _wire_single_wave(cue: unreal.SoundCue, wave: unreal.SoundWave):
    node = unreal.SoundNodeWavePlayer()
    node.set_editor_property("sound_wave", wave)
    cue.set_editor_property("first_node", node)


def _wire_random_waves(cue: unreal.SoundCue, waves: list):
    random_node = unreal.SoundNodeRandom()
    w = 1.0 / len(waves)
    random_node.set_editor_property("weights", [w] * len(waves))
    random_node.set_editor_property("randomize_without_replacement", True)

    child_nodes = []
    for wave in waves:
        wp = unreal.SoundNodeWavePlayer()
        wp.set_editor_property("sound_wave", wave)
        child_nodes.append(wp)

    random_node.set_editor_property("child_nodes", child_nodes)
    cue.set_editor_property("first_node", random_node)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def run():
    print("[MC2Cue] Collecting all SoundWave assets…")
    stem_map = collect_all_waves()
    print(f"[MC2Cue] Found {len(stem_map)} SoundWave assets.")

    # --- RSP-defined groups ---
    rsp_groups = load_rsp_groups(RSP_DIR)
    print(f"[MC2Cue] Loaded {len(rsp_groups)} RSP groups from {RSP_DIR}")

    covered_stems: set[str] = set()
    rsp_cue_tasks: list[tuple] = []  # (cue_dir, base_name, [waves])

    # RSP cues always go in /Game/Audio/VO/RSP/
    rsp_cue_dir = f"{AUDIO_ROOT}/VO/RSP"
    vo_class    = get_sound_class(f"{AUDIO_ROOT}/VO")

    for rsp_stem, wav_stems in rsp_groups.items():
        waves = []
        for ws in wav_stems:
            entry = stem_map.get(ws.lower())
            if entry:
                waves.append(entry[0])
                covered_stems.add(ws.lower())
        if waves:
            rsp_cue_tasks.append((rsp_cue_dir, rsp_stem, waves, vo_class))

    # --- Numeric-suffix groups for everything else ---
    suffix_groups = collect_waves_by_numeric_suffix(stem_map, covered_stems)
    suffix_tasks: list[tuple] = []
    for pkg_dir, bases in suffix_groups.items():
        sc = get_sound_class(pkg_dir)
        for base_name, waves in bases.items():
            suffix_tasks.append((pkg_dir, base_name, waves, sc))

    total   = len(rsp_cue_tasks) + len(suffix_tasks)
    created = 0
    skipped = 0

    print(f"[MC2Cue] Building {len(rsp_cue_tasks)} RSP cues + "
          f"{len(suffix_tasks)} suffix-grouped cues = {total} total.")

    with unreal.ScopedSlowTask(total, "Building Sound Cues") as task:
        task.make_dialog(True)

        for cue_dir, base_name, waves, sc in rsp_cue_tasks + suffix_tasks:
            if task.should_cancel():
                break
            task.enter_progress_frame(1, f"SC_{base_name}")
            ok = build_cue(cue_dir, base_name, waves, sc)
            if ok:
                created += 1
            else:
                skipped += 1

    print(f"[MC2Cue] Done. Created: {created}  Skipped (exist): {skipped}")


run()
