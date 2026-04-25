#!/usr/bin/env python3
"""
MC2 Data Converter: FIT + CSV -> UE5 Data Table CSVs
Generates:
  DT_Components.csv     from compbas.csv
  DT_MechChassis.csv    from atlas.csv, madcat.csv, ... (per-mech CSV files)
  DT_MechVariants.csv   from the VariantInfo blocks inside each mech CSV
  DT_VehicleTypes.csv   from APC1.fit, ArmoredCar.fit, ... vehicle FIT files
  DT_BuildingTypes.csv  from building FIT files
  DT_WeaponItems.csv    subset of DT_Components (weapons only)
  DT_Pilots.csv         from Pilots.csv

UE5 Data Table CSV format: first column is "---" (row name) or a key.
All floats use dot notation, all ints are plain, booleans are True/False.

Usage:
    python fit_convert.py <source_objects_dir> <output_dir>
    python fit_convert.py Source/Data/Objects/ ue5_datatables/
"""

import csv
import io
import os
import re
import sys
from pathlib import Path
from typing import Any


# ---------------------------------------------------------------------------
# FIT INI parser
# ---------------------------------------------------------------------------

def parse_fit(text: str) -> dict:
    """
    Parse a FIT INI file into a dict of { section_name: { key: value } }.
    Type prefixes (l, f, st, uc, b, c) are stripped; values are cast accordingly.
    Comment lines (//) are ignored.
    """
    result: dict[str, dict] = {}
    current_section = None

    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line or line.startswith("//") or line in ("FITini", "FITend", "FITEnd"):
            continue
        if line.startswith("[") and line.endswith("]"):
            current_section = line[1:-1]
            result.setdefault(current_section, {})
            continue
        if current_section is None:
            continue
        # Strip inline comments
        line = re.sub(r"//.*$", "", line).strip()
        if not line:
            continue

        # key = value, optional type prefix
        m = re.match(r"^([lfstucb]c?)\s+(\w+)\s*=\s*(.+)$", line)
        if not m:
            m = re.match(r"^(\w+)\s*=\s*(.+)$", line)
            if m:
                key, raw_val = m.group(1), m.group(2).strip()
                result[current_section][key] = _coerce(raw_val)
            continue

        type_prefix = m.group(1)
        key = m.group(2)
        raw_val = m.group(3).strip()
        result[current_section][key] = _coerce_typed(type_prefix, raw_val)

    return result


def _coerce(v: str) -> Any:
    v = v.strip().strip('"')
    if v.upper() == "TRUE":
        return True
    if v.upper() == "FALSE":
        return False
    try:
        return int(v)
    except ValueError:
        pass
    try:
        return float(v)
    except ValueError:
        pass
    return v


def _coerce_typed(prefix: str, v: str) -> Any:
    v = v.strip()
    if prefix == "st":
        return v.strip('"')
    if prefix == "b":
        return v.upper() == "TRUE"
    if prefix in ("l", "uc", "c"):
        try:
            return int(v)
        except ValueError:
            return v
    if prefix == "f":
        try:
            return float(v)
        except ValueError:
            return v
    return _coerce(v)


# ---------------------------------------------------------------------------
# compbas.csv -> DT_Components.csv
# ---------------------------------------------------------------------------

COMP_FIELDS = [
    "---", "MasterID", "Type", "Name", "CritHits", "RecycleTime",
    "Heat", "Weight", "Damage", "BattleRating", "RefitPoints", "Range",
    "LocHead", "LocCT", "LocLT", "LocRT", "LocLA", "LocRA", "LocLL", "LocRL",
    "MissileType", "SpecialFlags", "FXIndex", "AmmoMasterID",
    "LogisticsIcon1", "LogisticsIcon2", "StringIndex", "EncyclopediaIndex",
]

def convert_components(src: Path, out_dir: Path):
    rows = []
    with open(src, newline="", encoding="utf-8-sig") as f:
        reader = csv.reader(f)
        header = next(reader)  # "Component Table,Type,Name,..."
        next(reader)           # skip second header row if any
        for line in reader:
            if not line or not line[0].strip().isdigit():
                continue
            master_id = int(line[0])
            comp_type = line[1].strip()
            name      = line[2].strip()
            if comp_type.lower() in ("removed", "undefined"):
                continue
            row = {
                "---":            f"Comp_{master_id:03d}",
                "MasterID":       master_id,
                "Type":           comp_type,
                "Name":           name,
                "CritHits":       _safe_int(line[3]),
                "RecycleTime":    _safe_float(line[4]),
                "Heat":           _safe_float(line[5]),
                "Weight":         _safe_float(line[6]),
                "Damage":         _safe_float(line[7]),
                "BattleRating":   _safe_int(line[8]),
                "RefitPoints":    _safe_int(line[9]),
                "Range":          _safe_float(line[10]),
                "LocHead":        _bool_loc(line[11]),
                "LocCT":          _bool_loc(line[12]),
                "LocLT":          _bool_loc(line[13]),
                "LocRT":          _bool_loc(line[14]),
                "LocLA":          _bool_loc(line[15]),
                "LocRA":          _bool_loc(line[16]),
                "LocLL":          _bool_loc(line[17]),
                "LocRL":          _bool_loc(line[18]),
                "MissileType":    line[19].strip() if len(line) > 19 else "",
                "SpecialFlags":   _safe_int(line[20]) if len(line) > 20 else 0,
                "FXIndex":        _safe_int(line[21]) if len(line) > 21 else 0,
                "AmmoMasterID":   _safe_int(line[22]) if len(line) > 22 else 0,
                "LogisticsIcon1": line[23].strip() if len(line) > 23 else "",
                "LogisticsIcon2": line[24].strip() if len(line) > 24 else "",
                "StringIndex":    _safe_int(line[25]) if len(line) > 25 else 0,
                "EncyclopediaIndex": _safe_int(line[26]) if len(line) > 26 else 0,
            }
            rows.append(row)

    _write_csv(out_dir / "DT_Components.csv", COMP_FIELDS, rows)
    print(f"  DT_Components.csv: {len(rows)} components")


# ---------------------------------------------------------------------------
# Per-mech CSV -> DT_MechChassis.csv + DT_MechVariants.csv
# ---------------------------------------------------------------------------

CHASSIS_FIELDS = [
    "---", "MechName", "Tonnage", "HeatIndex", "LoadIndex", "TotalArmor",
    "MaxRunSpeed", "TorsoYawRate", "MaxTorsoYaw", "ChassisBR",
    "ChassisC_Bills", "MechParts", "MaxArmor", "DescIndex", "AbbrIndex",
    "EncyclopediaID", "HelpId", "AnimationName", "LittleIcon", "BigIcon",
    "IS_Head", "IS_LeftArm", "IS_RightArm", "IS_LeftTorso", "IS_RightTorso",
    "IS_CenterTorso", "IS_LeftLeg", "IS_RightLeg",
    "Armor_Head", "Armor_LA", "Armor_RA", "Armor_LT", "Armor_RT",
    "Armor_CT", "Armor_LL", "Armor_RL",
    "Armor_RearLT", "Armor_RearRT", "Armor_RearCT",
]

VARIANT_FIELDS = [
    "---", "MechName", "VariantID", "VariantName",
    "Item0", "Item1", "Item2", "Item3", "Item4", "Item5", "Item6", "Item7",
    "Item8", "Item9", "Item10", "Item11", "Item12", "Item13", "Item14", "Item15",
    "Item16", "Item17", "Item18", "Item19",
]


def convert_mech_csvs(objects_dir: Path, out_dir: Path):
    # All per-mech CSV files (lowercase name = chassis name)
    mech_csvs = [
        p for p in objects_dir.glob("*.csv")
        if p.stem.lower() not in (
            "compbas", "variants", "pilots", "badpilots", "hbstatz", "effects", "heat"
        )
    ]

    chassis_rows = []
    variant_rows = []

    for csv_path in sorted(mech_csvs):
        try:
            c_rows, v_rows = _parse_mech_csv(csv_path)
            chassis_rows.extend(c_rows)
            variant_rows.extend(v_rows)
        except Exception as e:
            print(f"  WARNING: {csv_path.name}: {e}")

    _write_csv(out_dir / "DT_MechChassis.csv", CHASSIS_FIELDS, chassis_rows)
    _write_csv(out_dir / "DT_MechVariants.csv", VARIANT_FIELDS, variant_rows)
    print(f"  DT_MechChassis.csv: {len(chassis_rows)} chassis")
    print(f"  DT_MechVariants.csv: {len(variant_rows)} variants")


def _parse_mech_csv(path: Path) -> tuple:
    """Parse a per-mech CSV file (atlas.csv, madcat.csv, etc.)."""
    with open(path, newline="", encoding="utf-8-sig") as f:
        lines = [row for row in csv.reader(f)]

    # Build a dict of label->value from the non-table rows.
    # The CSV uses groups of 3 columns: key, value, spacer (repeat across each row).
    kvs = {}
    for row in lines:
        # Try every column as a potential key; values are in the next non-empty column.
        i = 0
        while i < len(row):
            k = row[i].strip()
            if k and not k.startswith("//"):
                # Value is in the adjacent column if it exists
                v = row[i+1].strip() if i+1 < len(row) else ""
                if v and k:
                    kvs[k] = v
            i += 1

    mech_name = kvs.get("MechName", path.stem)

    # Find armor rows
    armor_is = []
    armor_pts = []
    for row in lines:
        if row and row[0].strip() == "BaseInternalStructure":
            armor_is = [_safe_int(x) for x in row[1:12]]
        if row and row[0].strip() == "BaseArmorPoints":
            armor_pts = [_safe_int(x) for x in row[1:12]]

    def ai(i): return armor_is[i] if i < len(armor_is) else 0
    def ap(i): return armor_pts[i] if i < len(armor_pts) else 0
    # Order: Head, LA, RA, LT, RT, CT, LL, RL, RearLT, RearRT, RearCT
    chassis_row = {
        "---":           mech_name,
        "MechName":      mech_name,
        "Tonnage":       _safe_float(kvs.get("Tonnage", "0")),
        "HeatIndex":     _safe_int(kvs.get("Heat Index", "0")),
        "LoadIndex":     _safe_int(kvs.get("Load Index", "0")),
        "TotalArmor":    _safe_int(kvs.get("Total Armor", "0")),
        "MaxRunSpeed":   _safe_float(kvs.get("MaxRunSpeed", "0")),
        "TorsoYawRate":  _safe_int(kvs.get("TorsoYawRate", "360")),
        "MaxTorsoYaw":   _safe_int(kvs.get("MaxTorsoYaw", "120")),
        "ChassisBR":     _safe_int(kvs.get("ChassisBR", "0")),
        "ChassisC_Bills":_safe_int(kvs.get("ChassisC-Bills", "0")),
        "MechParts":     _safe_int(kvs.get("MechParts", "1")),
        "MaxArmor":      _safe_int(kvs.get("MAX Armor", "0")),
        "DescIndex":     _safe_int(kvs.get("DescIndex", "0")),
        "AbbrIndex":     _safe_int(kvs.get("AbbrIndex", "0")),
        "EncyclopediaID":_safe_int(kvs.get("EncyclopediaID", "-1")),
        "HelpId":        _safe_int(kvs.get("HelpId", "-1")),
        "AnimationName": kvs.get("Animation", ""),
        "LittleIcon":    kvs.get("LittleIcon", ""),
        "BigIcon":       kvs.get("BigIcon", ""),
        "IS_Head":       ai(0), "IS_LeftArm": ai(1), "IS_RightArm": ai(2),
        "IS_LeftTorso":  ai(3), "IS_RightTorso": ai(4), "IS_CenterTorso": ai(5),
        "IS_LeftLeg":    ai(6), "IS_RightLeg": ai(7),
        "Armor_Head":    ap(0), "Armor_LA":   ap(1), "Armor_RA": ap(2),
        "Armor_LT":      ap(3), "Armor_RT":   ap(4), "Armor_CT": ap(5),
        "Armor_LL":      ap(6), "Armor_RL":   ap(7),
        "Armor_RearLT":  ap(8), "Armor_RearRT": ap(9), "Armor_RearCT": ap(10),
    }

    # Parse VariantInfo blocks
    variants = []
    in_variant = False
    cur_variant = {}
    item_ids = []

    for row in lines:
        if not row:
            continue
        tag = row[0].strip()
        if tag == "VariantID":
            if cur_variant:
                cur_variant["items"] = item_ids
                variants.append(cur_variant)
            cur_variant = {"VariantID": row[1].strip() if len(row) > 1 else "0000"}
            item_ids = []
            in_variant = True
        elif tag == "VariantName" and in_variant:
            cur_variant["VariantName"] = row[1].strip() if len(row) > 1 else ""
        elif re.match(r"^Item\d+MasterId$", tag) and in_variant:
            item_ids.append(_safe_int(row[1]) if len(row) > 1 else 0)

    if cur_variant:
        cur_variant["items"] = item_ids
        variants.append(cur_variant)

    v_rows = []
    for v in variants:
        vrow = {
            "---":        f"{mech_name}_{v.get('VariantID','0')}",
            "MechName":   mech_name,
            "VariantID":  v.get("VariantID", "0"),
            "VariantName": v.get("VariantName", ""),
        }
        items = v.get("items", [])
        for i in range(20):
            vrow[f"Item{i}"] = items[i] if i < len(items) else 0
        v_rows.append(vrow)

    return [chassis_row], v_rows


# ---------------------------------------------------------------------------
# Vehicle + Building FIT files -> DT_VehicleTypes.csv, DT_BuildingTypes.csv
# ---------------------------------------------------------------------------

VEHICLE_FIELDS = [
    "---", "Name", "AppearanceName", "ObjectTypeNum", "ExplosionObject",
    "DestroyedObject", "ExtentRadius", "MaxVelocity", "MaxAccel",
    "MaxTurretYawRate", "MaxTurretYaw", "MaxVehicleYawRate",
    "Tonnage", "BattleRating", "IconIndex",
    "Armor_Front", "Armor_Rear", "Armor_Left", "Armor_Right", "Armor_Turret",
    "IS_Front", "IS_Rear", "IS_Left", "IS_Right", "IS_Turret",
    "NumWeapons", "NumAmmo", "NumOther",
]

BUILDING_FIELDS = [
    "---", "Name", "AppearanceName", "ObjectTypeNum",
    "ExplosionObject", "DestroyedObject", "ExtentRadius",
]


def convert_vehicle_fits(objects_dir: Path, out_dir: Path):
    vehicle_rows = []
    building_rows = []

    for fit_path in sorted(objects_dir.glob("*.[Ff][Ii][Tt]")):
        sections = parse_fit(fit_path.read_text(encoding="utf-8", errors="replace"))
        obj_class = sections.get("ObjectClass", {})
        obj_type  = sections.get("ObjectType", {})
        obj_type_num = obj_class.get("ObjectTypeNum", 0)
        name = obj_type.get("Name", fit_path.stem)

        if "VehicleDynamics" in sections:
            dyn = sections["VehicleDynamics"]
            gen = sections.get("General", {})
            inv = sections.get("InventoryInfo", {})

            def armor(loc): return sections.get(loc, {}).get("CurArmorPoints", 0)
            def is_(loc):   return sections.get(loc, {}).get("CurInternalStructure", 0)

            vehicle_rows.append({
                "---":               name,
                "Name":              name,
                "AppearanceName":    obj_type.get("AppearanceName", name),
                "ObjectTypeNum":     obj_type_num,
                "ExplosionObject":   obj_type.get("ExplosionObject", 0),
                "DestroyedObject":   obj_type.get("DestroyedObject", 0),
                "ExtentRadius":      obj_type.get("ExtentRadius", 0.0),
                "MaxVelocity":       dyn.get("MaxVelocity", 0.0),
                "MaxAccel":          dyn.get("MaxAccel", 0.0),
                "MaxTurretYawRate":  dyn.get("MaxTurretYawRate", 0),
                "MaxTurretYaw":      dyn.get("MaxTurretYaw", 360),
                "MaxVehicleYawRate": dyn.get("MaxVehicleYawRate", 360),
                "Tonnage":           gen.get("CurTonnage", 0.0),
                "BattleRating":      gen.get("BattleRating", 0),
                "IconIndex":         gen.get("IconIndex", 0),
                "Armor_Front":       armor("Front"),
                "Armor_Rear":        armor("Rear"),
                "Armor_Left":        armor("Left"),
                "Armor_Right":       armor("Right"),
                "Armor_Turret":      armor("Turret"),
                "IS_Front":          is_("Front"),
                "IS_Rear":           is_("Rear"),
                "IS_Left":           is_("Left"),
                "IS_Right":          is_("Right"),
                "IS_Turret":         is_("Turret"),
                "NumWeapons":        inv.get("NumWeapons", 0),
                "NumAmmo":           inv.get("NumAmmo", 0),
                "NumOther":          inv.get("NumOther", 0),
            })

        elif "MechProfile" not in sections and obj_type:
            # Building-type object (no VehicleDynamics, no MechProfile)
            building_rows.append({
                "---":            name,
                "Name":           name,
                "AppearanceName": obj_type.get("AppearanceName", name),
                "ObjectTypeNum":  obj_type_num,
                "ExplosionObject":obj_type.get("ExplosionObject", 0),
                "DestroyedObject":obj_type.get("DestroyedObject", 0),
                "ExtentRadius":   obj_type.get("ExtentRadius", 0.0),
            })

    _write_csv(out_dir / "DT_VehicleTypes.csv", VEHICLE_FIELDS, vehicle_rows)
    _write_csv(out_dir / "DT_BuildingTypes.csv", BUILDING_FIELDS, building_rows)
    print(f"  DT_VehicleTypes.csv: {len(vehicle_rows)} vehicles")
    print(f"  DT_BuildingTypes.csv: {len(building_rows)} buildings/props")


# ---------------------------------------------------------------------------
# Pilots.csv -> DT_Pilots.csv
# ---------------------------------------------------------------------------

PILOT_FIELDS = [
    "---", "PilotName", "Rank", "Gunnery", "Piloting",
    "InitiativePips", "HouseID", "PortraitFile",
]

def convert_pilots(objects_dir: Path, out_dir: Path):
    pilots_path = objects_dir / "Pilots.csv"
    if not pilots_path.exists():
        return
    rows = []
    with open(pilots_path, newline="", encoding="utf-8-sig") as f:
        reader = csv.DictReader(f)
        for i, row in enumerate(reader):
            name = (row.get("Name") or row.get("PilotName") or f"Pilot_{i:03d}").strip()
            rows.append({
                "---":          name.replace(" ", "_"),
                "PilotName":    name,
                "Rank":         row.get("Rank", "").strip(),
                "Gunnery":      _safe_int(row.get("Gunnery", "4")),
                "Piloting":     _safe_int(row.get("Piloting", "5")),
                "InitiativePips": _safe_int(row.get("Initiative", "0")),
                "HouseID":      _safe_int(row.get("HouseID", "0")),
                "PortraitFile": row.get("Portrait", "").strip(),
            })
    _write_csv(out_dir / "DT_Pilots.csv", PILOT_FIELDS, rows)
    print(f"  DT_Pilots.csv: {len(rows)} pilots")


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _safe_int(v: Any) -> int:
    try:
        return int(str(v).replace(",", "").strip())
    except (ValueError, TypeError):
        return 0


def _safe_float(v: Any) -> float:
    try:
        return float(str(v).replace(",", "").strip())
    except (ValueError, TypeError):
        return 0.0


def _bool_loc(v: str) -> bool:
    v = str(v).strip()
    return not (v.upper() in ("NO", "0", "FALSE", "NA", "N/A", ""))


def _write_csv(path: Path, fields: list, rows: list):
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fields, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    objects_dir = Path(sys.argv[1])
    out_dir = Path(sys.argv[2])
    out_dir.mkdir(parents=True, exist_ok=True)

    print("Converting MC2 data to UE5 Data Table CSVs...")

    compbas = objects_dir / "compbas.csv"
    if compbas.exists():
        convert_components(compbas, out_dir)
    else:
        print("  WARNING: compbas.csv not found, skipping components")

    convert_mech_csvs(objects_dir, out_dir)
    convert_vehicle_fits(objects_dir, out_dir)
    convert_pilots(objects_dir, out_dir)

    print(f"\nOutput written to: {out_dir}")
    print("Next step: import these CSVs into UE5 as Data Tables using the matching row structs.")


if __name__ == "__main__":
    main()
