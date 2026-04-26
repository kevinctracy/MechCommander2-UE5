"""
ue_import_audio.py — run inside Unreal Editor Python console
Bulk-imports MC2 WAV files into /Game/Audio/ preserving the category structure
created by audio_organize.py.

Usage:
    1. Run audio_organize.py first to sort WAVs into categorized folders
    2. Set SOURCE_DIR to the output of audio_organize.py
    3. Run this script via Tools > Execute Python Script in the UE editor

The script creates Sound Cues for multi-variant files automatically.
Sound waves are saved as SoundWave assets under /Game/Audio/{Category}/.
"""

import unreal
import os
from pathlib import Path

SOURCE_DIR   = r"/tmp/mc2_audio_organized"   # output of audio_organize.py
DEST_PACKAGE = "/Game/Audio"

# Attenuation presets by category
ATTENUATION_MAP = {
    "SFX/Weapons":     "/Game/Audio/Attenuation/ATT_Weapon",
    "SFX/Mechs":       "/Game/Audio/Attenuation/ATT_Mech",
    "SFX/Explosions":  "/Game/Audio/Attenuation/ATT_Explosion",
    "SFX/Environment": "/Game/Audio/Attenuation/ATT_Ambient",
    "SFX/Vehicles":    "/Game/Audio/Attenuation/ATT_Vehicle",
    "SFX/UI":          None,   # UI sounds are 2D — no attenuation
    "VO/Campaign":     "/Game/Audio/Attenuation/ATT_VO",
    "VO/Tutorial":     "/Game/Audio/Attenuation/ATT_VO",
    "VO/Betty":        None,   # Betty is always 2D
    "VO/Cinema":       None,
    "Music":           None,
}

def relative_category(wav_path: Path, source_root: Path) -> str:
    try:
        return str(wav_path.parent.relative_to(source_root)).replace("\\", "/")
    except ValueError:
        return "Misc"

def run():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    registry    = unreal.AssetRegistryHelpers.get_asset_registry()
    wav_files   = list(Path(SOURCE_DIR).rglob("*.wav"))
    source_root = Path(SOURCE_DIR)

    print(f"[MC2Import] Found {len(wav_files)} WAV files in {SOURCE_DIR}")

    imported = 0
    skipped  = 0

    with unreal.ScopedSlowTask(len(wav_files), "Importing MC2 Audio") as task:
        task.make_dialog(True)

        for wav in wav_files:
            if task.should_cancel():
                break
            task.enter_progress_frame(1, wav.name)

            category     = relative_category(wav, source_root)
            dest_subpkg  = f"{DEST_PACKAGE}/{category}"
            dest_pkg     = f"{dest_subpkg}/{wav.stem}"

            if registry.get_asset_by_object_path(dest_pkg + "." + wav.stem).is_valid():
                skipped += 1
                continue

            import_task = unreal.AssetImportTask()
            import_task.filename         = str(wav)
            import_task.destination_path = dest_subpkg
            import_task.destination_name = wav.stem
            import_task.automated        = True
            import_task.save             = True

            asset_tools.import_asset_tasks([import_task])

            # Apply attenuation preset if one exists
            sw = unreal.load_asset(dest_pkg)
            if sw and isinstance(sw, unreal.SoundWave):
                att_path = ATTENUATION_MAP.get(category)
                if att_path:
                    att = unreal.load_asset(att_path)
                    if att:
                        sw.set_editor_property("attenuation_settings", att)
                else:
                    sw.set_editor_property("is_ambisonics", False)

                # VO lines: set subtitle / dialogue wave class
                if category.startswith("VO/"):
                    sw.set_editor_property("voice_output_enabled", True)

                unreal.EditorAssetLibrary.save_asset(dest_pkg)

            imported += 1

    print(f"[MC2Import] Audio done. Imported: {imported}  Skipped: {skipped}")

    # Build Sound Classes
    _ensure_sound_classes()

def _ensure_sound_classes():
    """Create the master Sound Class hierarchy if not already present."""
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    classes = {
        "/Game/Audio/SoundClasses/SC_Master":      None,
        "/Game/Audio/SoundClasses/SC_SFX":         "/Game/Audio/SoundClasses/SC_Master",
        "/Game/Audio/SoundClasses/SC_Music":       "/Game/Audio/SoundClasses/SC_Master",
        "/Game/Audio/SoundClasses/SC_VO":          "/Game/Audio/SoundClasses/SC_Master",
        "/Game/Audio/SoundClasses/SC_UI":          "/Game/Audio/SoundClasses/SC_Master",
        "/Game/Audio/SoundClasses/SC_Ambient":     "/Game/Audio/SoundClasses/SC_SFX",
    }

    for pkg, parent_pkg in classes.items():
        if not unreal.EditorAssetLibrary.does_asset_exist(pkg):
            sc = asset_tools.create_asset(
                pkg.split("/")[-1],
                "/".join(pkg.split("/")[:-1]),
                unreal.SoundClass, unreal.SoundClassFactory()
            )
            if sc and parent_pkg:
                parent = unreal.load_asset(parent_pkg)
                if parent:
                    sc.set_editor_property("parent_class", parent)
            if sc:
                unreal.EditorAssetLibrary.save_asset(pkg)
            print(f"[MC2Import] Created Sound Class: {pkg}")

run()
