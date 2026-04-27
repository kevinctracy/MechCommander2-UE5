"""
ue_import_data_tables.py — run inside Unreal Editor Python console
Imports CSV files produced by fit_convert.py into UE5 Data Table assets.

Prerequisites:
    1. Run fit_convert.py offline:
         python fit_convert.py Source/Data/Objects/ /tmp/mc2_datatables/
    2. Ensure MC2DataTableRows.h is compiled (Data Table row structs must exist)
    3. Run via Tools > Execute Python Script with the UE project open

Config: set CSV_DIR to the fit_convert.py output directory.
"""

import unreal
from pathlib import Path

# ── Config ────────────────────────────────────────────────────────────────────
CSV_DIR          = r"/tmp/mc2_datatables"
DATATABLES_PKG   = "/Game/Data"
MODULE           = "MechCommander2"

# CSV filename → (UE DataTable asset name, row struct class name)
TABLE_MAP = {
    "DT_Components.csv":         ("DT_Components",        "MC2ComponentRow"),
    "DT_MechChassis.csv":        ("DT_MechChassis",       "MC2MechChassisRow"),
    "DT_MechVariants.csv":       ("DT_MechVariants",      "MC2MechVariantRow"),
    "DT_VehicleTypes.csv":       ("DT_VehicleTypes",      "MC2VehicleTypeRow"),
    "DT_BuildingTypes.csv":      ("DT_BuildingTypes",     "MC2BuildingTypeRow"),
    "DT_Pilots.csv":             ("DT_Pilots",            "MC2PilotRow"),
    "DT_CampaignGroups.csv":     ("DT_CampaignGroups",    "MC2CampaignGroupRow"),
    "DT_CampaignMissions.csv":   ("DT_CampaignMissions",  "MC2CampaignMissionRow"),
    "DT_TutorialGroups.csv":     ("DT_TutorialGroups",    "MC2CampaignGroupRow"),
    "DT_TutorialMissions.csv":   ("DT_TutorialMissions",  "MC2CampaignMissionRow"),
    "DT_CameraSettings.csv":     ("DT_CameraSettings",    "MC2CameraSettingsRow"),
    "DT_TeamColors.csv":         ("DT_TeamColors",        "MC2TeamColorRow"),
}


def get_row_struct(struct_name: str):
    """Load row struct class from the game module."""
    class_path = f"/Script/{MODULE}.{struct_name}"
    obj = unreal.load_object(None, class_path)
    if not obj:
        unreal.log_warning(f'[MC2DT] Row struct not found: {class_path}')
    return obj


def import_csv_as_data_table(csv_path: str, dt_name: str, row_struct) -> bool:
    """
    Import a CSV file as a UE DataTable asset.
    If the asset already exists it is re-imported (updated in place).
    Returns True on success.
    """
    pkg_path = f"{DATATABLES_PKG}/{dt_name}"

    # Re-import if already exists
    if unreal.EditorAssetLibrary.does_asset_exist(pkg_path):
        existing = unreal.EditorAssetLibrary.load_asset(pkg_path)
        if existing and isinstance(existing, unreal.DataTable):
            try:
                result = unreal.DataTableFunctionLibrary.fill_data_table_from_csv_file(
                    existing, csv_path)
                if result:
                    unreal.EditorAssetLibrary.save_asset(pkg_path, only_if_is_dirty=False)
                    return True
                unreal.log_warning(f'[MC2DT] FillFromCSV failed for {dt_name}, trying reimport')
            except Exception:
                pass

    # Fresh import via AssetImportTask
    task = unreal.AssetImportTask()
    task.filename         = csv_path
    task.destination_path = DATATABLES_PKG
    task.destination_name = dt_name
    task.automated        = True
    task.save             = True
    task.replace_existing = True

    # DataTable factory options — set row struct
    if row_struct:
        factory = unreal.CSVImportFactory()
        factory.automated_import_settings.import_type = unreal.CSVImportType.ECSV_DATA_TABLE
        factory.automated_import_settings.data_table_row_type = row_struct
        task.factory = factory

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    if unreal.EditorAssetLibrary.does_asset_exist(pkg_path):
        return True

    # Fallback: create DataTable asset manually and fill from CSV
    try:
        at = unreal.AssetToolsHelpers.get_asset_tools()
        dt_factory = unreal.DataTableFactory()
        if row_struct:
            dt_factory.struct = row_struct
        new_dt = at.create_asset(dt_name, DATATABLES_PKG, unreal.DataTable, dt_factory)
        if new_dt:
            unreal.DataTableFunctionLibrary.fill_data_table_from_csv_file(new_dt, csv_path)
            unreal.EditorAssetLibrary.save_asset(pkg_path, only_if_is_dirty=False)
            return True
    except Exception as e:
        unreal.log_warning(f'[MC2DT] Fallback create failed for {dt_name}: {e}')

    return False


def run():
    csv_dir = Path(CSV_DIR)
    if not csv_dir.exists():
        unreal.log_error(f'[MC2DT] CSV directory not found: {CSV_DIR}')
        unreal.log_error('[MC2DT] Run fit_convert.py first:')
        unreal.log_error('[MC2DT]   python fit_convert.py Source/Data/Objects/ /tmp/mc2_datatables/')
        return

    # Ensure /Game/Data package exists
    unreal.EditorAssetLibrary.make_directory(DATATABLES_PKG)

    print(f'[MC2DT] Importing Data Tables from {CSV_DIR}')
    success = 0
    failed  = 0

    with unreal.ScopedSlowTask(len(TABLE_MAP), 'Importing MC2 Data Tables') as task:
        task.make_dialog(True)

        for csv_filename, (dt_name, struct_name) in TABLE_MAP.items():
            if task.should_cancel():
                break
            task.enter_progress_frame(1, dt_name)

            csv_path = str(csv_dir / csv_filename)
            if not Path(csv_path).exists():
                unreal.log_warning(f'[MC2DT] CSV not found: {csv_path}  (skipping)')
                failed += 1
                continue

            row_struct = get_row_struct(struct_name)
            ok = import_csv_as_data_table(csv_path, dt_name, row_struct)

            if ok:
                print(f'[MC2DT]   ✓ {dt_name}  ← {csv_filename}')
                success += 1
            else:
                unreal.log_warning(f'[MC2DT]   ✗ {dt_name}  ← {csv_filename}')
                failed += 1

    print(f'\n[MC2DT] Done: {success} imported, {failed} failed')
    if success > 0:
        print('[MC2DT] Data Tables are in Content Browser under Game/Data/')
    print('[MC2DT] Verify row counts match:')
    print('[MC2DT]   DT_Components       ~80 rows')
    print('[MC2DT]   DT_MechChassis      ~34 rows')
    print('[MC2DT]   DT_MechVariants     ~42 rows')
    print('[MC2DT]   DT_VehicleTypes     ~67 rows')
    print('[MC2DT]   DT_BuildingTypes    ~1088 rows')
    print('[MC2DT]   DT_Pilots           ~51 rows')
    print('[MC2DT]   DT_CampaignGroups   13 rows')
    print('[MC2DT]   DT_CampaignMissions 24 rows')
    print('[MC2DT]   DT_TutorialGroups   1 row')
    print('[MC2DT]   DT_TutorialMissions 5 rows')
    print('[MC2DT]   DT_CameraSettings   1 row')
    print('[MC2DT]   DT_TeamColors       112 rows (2 tables × 56 colors)')


run()
