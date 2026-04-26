"""
ue_build_sound_mix.py — run inside Unreal Editor Python console
Creates the MC2 Sound Mix asset with per-class volume settings and
wires it to the Sound Classes created by ue_import_audio.py.

Run AFTER ue_import_audio.py (which creates the SC_* Sound Class assets).

The Sound Mix controls master volume levels for each audio category.
Players adjust these via WBP_Options → UMC2GameUserSettings volume sliders.

Output:
    /Game/Audio/Mix/SM_MC2Master   — the Sound Mix asset
"""

import unreal

SOUND_CLASS_PATH = "/Game/Audio/SoundClasses"
SOUND_MIX_DIR    = "/Game/Audio/Mix"
SOUND_MIX_NAME   = "SM_MC2Master"

# Default volumes: all at 1.0 except UI (slightly quieter during gameplay)
CLASS_VOLUMES = {
    "SC_Master":  1.0,
    "SC_SFX":     1.0,
    "SC_Music":   0.8,
    "SC_VO":      1.0,
    "SC_UI":      0.75,
    "SC_Ambient": 0.9,
}


def build_mix_entry(class_name: str, volume: float):
    """Build an FSoundClassAdjuster for one Sound Class."""
    sc_path  = f"{SOUND_CLASS_PATH}/{class_name}"
    sc_asset = unreal.load_asset(sc_path)
    if not sc_asset:
        unreal.log_warning(f"[MC2Mix] Sound Class not found, skipping: {sc_path}")
        return None

    adjuster = unreal.SoundClassAdjuster()
    adjuster.set_editor_property("sound_class_object", sc_asset)
    adjuster.set_editor_property("volume_adjuster",    volume)
    adjuster.set_editor_property("pitch_adjuster",     1.0)
    adjuster.set_editor_property("applies_to_children", True)
    return adjuster


def run():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mix_pkg     = f"{SOUND_MIX_DIR}/{SOUND_MIX_NAME}"

    if unreal.EditorAssetLibrary.does_asset_exist(mix_pkg):
        print(f"[MC2Mix] Sound Mix already exists: {mix_pkg}  (skipping creation)")
        mix = unreal.load_asset(mix_pkg)
    else:
        mix = asset_tools.create_asset(
            SOUND_MIX_NAME, SOUND_MIX_DIR,
            unreal.SoundMix, unreal.SoundMixFactory()
        )
        if not mix:
            unreal.log_error("[MC2Mix] Failed to create Sound Mix asset.")
            return
        print(f"[MC2Mix] Created Sound Mix: {mix_pkg}")

    adjusters = []
    for class_name, volume in CLASS_VOLUMES.items():
        adj = build_mix_entry(class_name, volume)
        if adj:
            adjusters.append(adj)

    mix.set_editor_property("sound_class_effects", adjusters)

    # Fade settings: short fades for SFX, longer for music
    mix.set_editor_property("fade_in_time",  0.1)
    mix.set_editor_property("fade_out_time", 0.5)

    unreal.EditorAssetLibrary.save_asset(mix_pkg)

    # Push as the active mix
    unreal.SoundMixLibrary.push_sound_mix_modifier(
        unreal.EditorLevelLibrary.get_editor_world(), mix)

    print(f"[MC2Mix] Sound Mix configured with {len(adjusters)} class adjusters.")
    print(f"[MC2Mix] Assign SM_MC2Master as the DefaultBaseSoundMix in Project Settings > Audio.")


run()
