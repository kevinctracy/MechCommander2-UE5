"""
ue_import_txm_pngs.py — run inside Unreal Editor Python console
Imports extracted TXM PNG files (output of txm_extract.py) into /Game/Textures/TXM/.

Run txm_extract.py first to convert the 100 .txm runtime textures to PNG.

These textures are distinct from the source .tga files (which ue_import_textures.py handles).
TXM textures are runtime-only UI and effect textures compiled into the original game's
PAK files (64×64 BGRA palettised sprites from the FIT/txmmgr pipeline).

Compression rules:
  - All TXM PNGs → TC_EditorIcon (small, UI, no mipmaps) unless tagged _n
"""

import unreal
from pathlib import Path

SOURCE_DIR   = r"/tmp/mc2_txm_pngs"   # output of txm_extract.py
DEST_PACKAGE = "/Game/Textures/TXM"


def run():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    registry    = unreal.AssetRegistryHelpers.get_asset_registry()
    png_files   = list(Path(SOURCE_DIR).rglob("*.png"))

    print(f"[MC2TXM] Found {len(png_files)} PNG files in {SOURCE_DIR}")

    imported = 0
    skipped  = 0

    with unreal.ScopedSlowTask(len(png_files), "Importing TXM Textures") as task:
        task.make_dialog(True)

        for png in png_files:
            if task.should_cancel():
                break
            task.enter_progress_frame(1, png.name)

            stem     = png.stem
            dest_pkg = f"{DEST_PACKAGE}/{stem}"

            if registry.get_asset_by_object_path(dest_pkg + "." + stem).is_valid():
                skipped += 1
                continue

            import_task = unreal.AssetImportTask()
            import_task.filename         = str(png)
            import_task.destination_path = DEST_PACKAGE
            import_task.destination_name = stem
            import_task.automated        = True
            import_task.save             = False

            asset_tools.import_asset_tasks([import_task])

            tex = unreal.load_asset(dest_pkg)
            if tex and isinstance(tex, unreal.Texture2D):
                # TXM sprites are tiny UI textures — no mipmaps needed
                low = stem.lower()
                if "_n." in low or "_normal." in low:
                    comp = unreal.TextureCompressionSettings.TC_NORMALMAP
                    tex.set_editor_property("srgb", False)
                else:
                    comp = unreal.TextureCompressionSettings.TC_EDITOR_ICON
                    tex.set_editor_property("srgb", True)

                tex.set_editor_property("compression_settings", comp)
                tex.set_editor_property("mip_gen_settings",
                    unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
                tex.set_editor_property("never_stream", True)

                unreal.EditorAssetLibrary.save_asset(dest_pkg)

            imported += 1

    print(f"[MC2TXM] Done. Imported: {imported}  Skipped: {skipped}")


run()
