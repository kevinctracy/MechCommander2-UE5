"""
ue_import_textures.py — run inside Unreal Editor Python console
Bulk-imports all MC2 TGA textures into /Game/Textures/ with correct compression settings.

Usage (Unreal Editor > Tools > Execute Python Script):
    Set SOURCE_DIR to your extracted TGA folder before running.

Compression rules:
  - Files ending in _n.tga or _normal.tga → TC_Normalmap (BC5)
  - Files ending in _e.tga (emissive) → TC_HDR
  - All others → TC_Default (BC3 for RGBA, BC1 for RGB)

The script skips files already imported (checks asset registry).
"""

import unreal
import os
from pathlib import Path

SOURCE_DIR   = r"/Volumes/projects/personal/MechCommander2-Source/Source/Data/Textures"
DEST_PACKAGE = "/Game/Textures"

# Subdirectory organization matching audio_organize.py conventions:
#   /Game/Textures/Mechs/      — mech chassis textures
#   /Game/Textures/Terrain/    — terrain/landscape layer textures
#   /Game/Textures/Buildings/  — building/prop textures
#   /Game/Textures/UI/         — UI / FIT interface textures
#   /Game/Textures/Effects/    — particle/effect sheet textures
#   /Game/Textures/Misc/       — everything else

CATEGORY_RULES = [
    (lambda n: any(m in n for m in ["mech", "atlas", "thor", "mad", "timber", "catap"]), "Mechs"),
    (lambda n: any(m in n for m in ["terrain", "dirt", "grass", "rock", "snow", "water", "sand"]), "Terrain"),
    (lambda n: any(m in n for m in ["build", "base", "turret", "gate", "wall", "tower"]), "Buildings"),
    (lambda n: any(m in n for m in ["button", "panel", "screen", "hud", "icon", "logo"]), "UI"),
    (lambda n: any(m in n for m in ["smoke", "fire", "expl", "spark", "fx", "glow"]), "Effects"),
]

def categorize(filename: str) -> str:
    low = filename.lower()
    for rule, cat in CATEGORY_RULES:
        if rule(low):
            return cat
    return "Misc"

def compression_for(filename: str) -> unreal.TextureCompressionSettings:
    low = filename.lower()
    if "_n." in low or "_normal." in low or low.endswith("_n.tga"):
        return unreal.TextureCompressionSettings.TC_NORMALMAP
    if "_e." in low or "emissive" in low:
        return unreal.TextureCompressionSettings.TC_HDR
    return unreal.TextureCompressionSettings.TC_DEFAULT

def run():
    asset_tools  = unreal.AssetToolsHelpers.get_asset_tools()
    registry     = unreal.AssetRegistryHelpers.get_asset_registry()
    tga_files    = list(Path(SOURCE_DIR).rglob("*.tga"))

    print(f"[MC2Import] Found {len(tga_files)} TGA files in {SOURCE_DIR}")

    imported = 0
    skipped  = 0

    with unreal.ScopedSlowTask(len(tga_files), "Importing MC2 Textures") as task:
        task.make_dialog(True)

        for tga in tga_files:
            if task.should_cancel():
                break
            task.enter_progress_frame(1, str(tga.name))

            stem     = tga.stem
            category = categorize(stem)
            dest_pkg = f"{DEST_PACKAGE}/{category}/{stem}"

            # Skip if already imported
            if registry.get_asset_by_object_path(dest_pkg + "." + stem).is_valid():
                skipped += 1
                continue

            # Set up task
            import_task = unreal.AssetImportTask()
            import_task.filename      = str(tga)
            import_task.destination_path  = f"{DEST_PACKAGE}/{category}"
            import_task.destination_name  = stem
            import_task.automated     = True
            import_task.save          = False

            # Apply compression options
            opts = unreal.TextureImportOptions()
            opts.compression_settings = compression_for(stem)
            import_task.options = opts

            asset_tools.import_asset_tasks([import_task])

            # Apply sRGB / mip settings post-import
            tex = unreal.load_asset(dest_pkg)
            if tex and isinstance(tex, unreal.Texture2D):
                tex.set_editor_property("srgb", "_n." not in stem.lower())
                tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
                unreal.EditorAssetLibrary.save_asset(dest_pkg)

            imported += 1

    print(f"[MC2Import] Done. Imported: {imported}  Skipped (already exist): {skipped}")

run()
