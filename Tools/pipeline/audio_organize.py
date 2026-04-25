#!/usr/bin/env python3
"""
MC2 Audio Organizer
Categorizes and copies WAV files into a UE5-ready folder structure.

Categories:
  SFX/Weapons/        sfx_weapon_*, sfx_ac_*, sfx_lrm_*, sfx_ppc_*, sfx_laser_*
  SFX/Mechs/          sfx_mech_*, sfx_pilot_*
  SFX/Vehicles/       sfx_vehicle_*, sfx_copter_*, sfx_heli_*
  SFX/Explosions/     sfx_explode_*, sfx_impact_*, sfx_building_*
  SFX/Environment/    sfx_ambient_*, sfx_rain_*, sfx_wind_*
  SFX/UI/             sfx_int_*, sfx_ui_*, sfx_click_*, CHOICE_*.wav
  VO/Campaign/        mc2_NN_*.wav  (campaign mission voice over)
  VO/Tutorial/        tut_*.wav
  VO/Betty/           Betty/ subdirectory (status announcements)
  VO/Cinema/          Cinema*.wav
  Music/              W0*.wav (music tracks)
  Misc/               anything unclassified

Usage:
    python audio_organize.py <sound_dir> <output_dir>
    python audio_organize.py Source/Data/Sound/ ue5_audio/
"""

import re
import shutil
import sys
from pathlib import Path


CATEGORY_RULES = [
    # (category_path, [patterns...])  — patterns are case-insensitive regex matches on filename stem
    ("SFX/Weapons",      [r"sfx_weapon", r"sfx_ac\d", r"sfx_lrm", r"sfx_ppc",
                          r"sfx_laser", r"sfx_lazer", r"sfx_lazers",
                          r"sfx_orbital", r"sfx_gauss", r"sfx_flamer", r"sfx_missile",
                          r"sfx_artillery", r"sfx_mgun", r"sfx_autocannon",
                          r"sfx_rocket", r"sfx_srm"]),
    ("SFX/Mechs",        [r"sfx_mech", r"sfx_pilot", r"sfx_fall", r"sfx_jump", r"sfx_heat",
                          r"sfx_footfall", r"sfx_gate", r"sfx_radar"]),
    ("SFX/Vehicles",     [r"sfx_vehicle", r"sfx_copter", r"sfx_heli", r"sfx_tank",
                          r"sfx_apc", r"sfx_truck", r"^rtruck", r"^scopter",
                          r"^helicopter", r"^copter"]),
    ("SFX/Explosions",   [r"sfx_explode", r"sfx_impact", r"sfx_building",
                          r"sfx_explo", r"sfx_death"]),
    ("SFX/Environment",  [r"sfx_ambient", r"sfx_rain", r"sfx_wind", r"sfx_fire",
                          r"sfx_water"]),
    ("SFX/UI",           [r"sfx_int_", r"sfx_ui_", r"sfx_click", r"^choice_\d",
                          r"^node_\d"]),
    # Pilot voice: PLT1_ through PLT26_ prefixes
    ("VO/Pilots",        [r"^plt\d+_", r"^artillery_", r"^mlayer_"]),
    ("VO/Tutorial",      [r"^tut_"]),
    ("VO/Cinema",        [r"^cinema\d"]),
    ("Music",            [r"^w0\d", r"music"]),
]


def categorize(filename: str) -> str:
    lower = filename.lower()
    for category, patterns in CATEGORY_RULES:
        for pat in patterns:
            if re.search(pat, lower):
                return category

    # Mission VO: mc2_NN_x or MC2_NNA format
    if re.match(r"mc2_\d+_", lower) or re.match(r"mc2_\d+[a-z]", lower):
        return "VO/Campaign"

    return "Misc"


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    sound_dir = Path(sys.argv[1])
    out_dir   = Path(sys.argv[2])

    wav_files = list(sound_dir.rglob("*.wav")) + list(sound_dir.rglob("*.WAV"))
    print(f"Found {len(wav_files)} WAV files")

    counts: dict = {}
    manifest_rows = []

    for src in sorted(wav_files):
        # Betty subdirectory -> VO/Betty/
        if src.parent.name.lower() == "betty":
            category = "VO/Betty"
        else:
            category = categorize(src.name)

        dst_dir = out_dir / category
        dst_dir.mkdir(parents=True, exist_ok=True)
        dst = dst_dir / src.name.upper()  # normalize to uppercase for UE consistency

        shutil.copy2(src, dst)
        counts[category] = counts.get(category, 0) + 1
        manifest_rows.append(f"{category},{src.name},{dst.relative_to(out_dir)}")

    # Print summary
    print("\nAudio organized:")
    for cat in sorted(counts):
        print(f"  {cat}: {counts[cat]} files")
    print(f"\nTotal: {sum(counts.values())} files copied to {out_dir}")

    # Write UE import manifest
    manifest_path = out_dir / "audio_import_manifest.csv"
    with open(manifest_path, "w") as f:
        f.write("Category,OriginalName,UEPath\n")
        f.write("\n".join(manifest_rows))
    print(f"Import manifest: {manifest_path}")

    # Write Sound Class suggestions
    sc_path = out_dir / "soundclass_suggestions.txt"
    with open(sc_path, "w") as f:
        f.write("Recommended UE5 Sound Class hierarchy:\n")
        f.write("  Master\n")
        f.write("    Music       (SoundClass_Music)   — W0x.wav, adjust volume independently\n")
        f.write("    SFX         (SoundClass_SFX)\n")
        f.write("      Weapons   (SoundClass_Weapons)\n")
        f.write("      Mechs     (SoundClass_Mechs)\n")
        f.write("      Vehicles  (SoundClass_Vehicles)\n")
        f.write("      UI        (SoundClass_UI)      — non-spatialized, 2D\n")
        f.write("    VO          (SoundClass_VO)      — priority, non-spatialized\n")
        f.write("\nSuggested Sound Cue groupings:\n")
        f.write("  SC_WeaponFire_AC20    — AC20 fire variants (AC20hit.wav etc)\n")
        f.write("  SC_WeaponFire_LRM     — LRM launch + flight + impact\n")
        f.write("  SC_MechFootstep_Heavy — heavy mech footfall variants\n")
        f.write("  SC_Explosion_Large    — large explosion variants\n")
    print(f"Sound class guide: {sc_path}")


if __name__ == "__main__":
    main()
