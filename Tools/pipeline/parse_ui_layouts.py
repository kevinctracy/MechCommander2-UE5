#!/usr/bin/env python3
"""
parse_ui_layouts.py — MC2 Art FIT → JSON layout reference

Converts every *.fit file in Source/Data/Art/ into a JSON file that describes
the positions, sizes, textures, colors, and animations of every UI element.
Use these JSON files as a blueprint when building WBP visual layouts in UMG.

Usage:
    python3 parse_ui_layouts.py <art_fit_dir> <output_dir>
    python3 parse_ui_layouts.py Source/Data/Art/ /tmp/mc2_ui_layouts/

Output:
    One <screen_name>.json per .fit file.

UMG setup note
--------------
MC2 original canvas: 640 × 480 pixels (some fullscreen elements use 800 × 600).
In Unreal Engine, set the UMG design resolution to match:
    Project Settings → User Interface → UI Scale → Reference Resolution = 640×480
    Scale Rule = Shortest Side
Then use absolute pixel positions from these JSON files directly as UMG slot
offsets (Horizontal/VerticalBox → Canvas Panel → Position + Size).
"""

import json
import re
import sys
from pathlib import Path
from typing import Any


# ---------------------------------------------------------------------------
# FIT parser (self-contained — mirrors fit_convert.py's parse_fit)
# ---------------------------------------------------------------------------

def parse_fit(text: str) -> dict:
    """Return {section: {key: value}} from FIT INI text."""
    result: dict[str, dict] = {}
    current = None

    for raw in text.splitlines():
        line = re.sub(r"//.*$", "", raw).strip()
        if not line or line in ("FITini", "FITend", "FITEnd", "Fitend", "FitEnd"):
            continue
        if line.startswith("[") and line.endswith("]"):
            current = line[1:-1]
            result.setdefault(current, {})
            continue
        if current is None:
            continue

        # f[N] ArrayKey = v0,v1,... (array values)
        m = re.match(r"^f\[\d+\]\s+(\w+)\s*=\s*(.+)$", line)
        if m:
            try:
                result[current][m.group(1)] = [float(x.strip()) for x in m.group(2).split(",")]
            except ValueError:
                result[current][m.group(1)] = m.group(2).strip()
            continue

        # Type-prefixed: ul/uc/st/l/f/b key = value
        m = re.match(r"^(ul|uc|st|[lfbc])\s+(\w+)\s*=\s*(.+)$", line)
        if m:
            result[current][m.group(2)] = _cast(m.group(1), m.group(3).strip())
            continue

        # Plain: key = value
        m = re.match(r"^(\w+)\s*=\s*(.+)$", line)
        if m:
            result[current][m.group(1)] = _auto(m.group(2).strip())

    return result


def _strip_garbage(v: str) -> str:
    """Strip trailing non-numeric/non-hex junk from values like '48d', '129|', '189m'."""
    return re.sub(r"[^0-9a-fA-FxX.\-]+$", "", v.strip())


def _cast(prefix: str, v: str) -> Any:
    if prefix == "st":
        return v.strip('"')
    if prefix == "b":
        return v.upper() in ("TRUE", "1")
    if prefix in ("l", "uc", "ul"):
        try:
            return int(v, 0)  # handles 0x hex literals
        except (ValueError, TypeError):
            pass
        # Tolerate trailing junk like '48d', '129|' — strip non-decimal chars
        try:
            return int(re.sub(r"[^0-9\-]+$", "", v))
        except (ValueError, TypeError):
            return v
    if prefix == "f":
        try:
            return float(_strip_garbage(v))
        except ValueError:
            return v
    return _auto(v)


def _auto(v: str) -> Any:
    v = v.strip().strip('"')
    if v.upper() == "TRUE":
        return True
    if v.upper() == "FALSE":
        return False
    # hex literal
    if re.match(r"^0[xX][0-9a-fA-F]+$", v):
        try:
            return int(v, 16)
        except ValueError:
            pass
    clean = _strip_garbage(v)
    try:
        return int(clean)
    except ValueError:
        pass
    try:
        return float(clean)
    except ValueError:
        pass
    return v


# ---------------------------------------------------------------------------
# Element type names
# ---------------------------------------------------------------------------

ELEMENT_TYPE_NAMES = {
    0: "background",
    1: "button",
    2: "text",
    3: "meter",      # attribute/progress bar
    4: "rect",
    5: "static",     # static image
    6: "edit",       # text input
    7: "list",
    8: "dropdown",
}

ANIM_SECTION_RE = re.compile(r"^Animation", re.IGNORECASE)

# Sections that are visual element groups — pattern: {prefix}{index}
ELEMENT_GROUP_PREFIXES = (
    "Element", "AnimObject", "Button", "ScrollButton",
    "Static", "Rect", "Text", "Slider", "Edit", "List",
    "Dropdown", "ComboBox", "Entry", "Icon",
)


# ---------------------------------------------------------------------------
# Extraction helpers
# ---------------------------------------------------------------------------

def _rect_from_section(s: dict) -> dict | None:
    """Return {left, top, width, height} from a section dict, or None."""
    if "Left" in s and "Top" in s:
        return {
            "left":   int(s.get("Left",   0)),
            "top":    int(s.get("Top",    0)),
            "width":  int(s.get("Width",  0)),
            "height": int(s.get("Height", 0)),
        }
    if "XLocation" in s or "XLocation" in {k.lower(): k for k in s}:
        xloc = s.get("XLocation") or s.get("Xlocation") or s.get("xlocation") or 0
        yloc = s.get("YLocation") or s.get("Ylocation") or s.get("ylocation") or 0
        return {
            "left":   int(xloc),
            "top":    int(yloc),
            "width":  int(s.get("Width",   0)),
            "height": int(s.get("Height",  0)),
        }
    return None


def _uv(s: dict, idx: int) -> dict | None:
    tl = (s.get(f"uvtlx{idx}"), s.get(f"uvtly{idx}"))
    br = (s.get(f"uvbrx{idx}"), s.get(f"uvbry{idx}"))
    if tl[0] is None:
        return None
    return {
        "tl": [float(tl[0]), float(tl[1])],
        "br": [float(br[0]), float(br[1])],
    }


def _texture_state(s: dict, state_key: str, idx: int) -> dict | None:
    """Return {art, uv_tl, uv_br} for a button state."""
    art = s.get(state_key)
    if not art:
        return None
    uv = _uv(s, idx)
    out: dict = {"art": art}
    if uv:
        out.update({"uv_tl": uv["tl"], "uv_br": uv["br"]})
    # Sprite-sheet UV offsets (U/VNormal style)
    u_key = f"U{state_key.replace('Art', '')}"
    v_key = f"V{state_key.replace('Art', '')}"
    if u_key in s:
        out["sprite_uv"] = [int(s[u_key]), int(s.get(v_key, 0))]
        out["sprite_size"] = [int(s.get("UWidth", 0)), int(s.get("VHeight", 0))]
    return out


def _extract_planes(s: dict) -> list:
    """Extract multi-plane background art from Element sections."""
    planes = []
    n = int(s.get("NumPlanes", 1))
    for i in range(n):
        art = s.get(f"Art{i}")
        if art:
            entry: dict = {"art": art}
            uv = _uv(s, i)
            if uv:
                entry.update({"uv_tl": uv["tl"], "uv_br": uv["br"]})
            planes.append(entry)
    return planes


def _extract_background(s: dict) -> dict:
    return {
        "type":   "background",
        "rect":   _rect_from_section(s),
        "planes": _extract_planes(s),
    }


def _extract_button(s: dict) -> dict:
    out: dict = {
        "type":    "button",
        "rect":    _rect_from_section(s),
    }
    if "Message" in s:
        out["message"] = s["Message"]
    if "TextID" in s:
        out["text_id"] = s["TextID"]
    if "Font" in s:
        out["font_id"] = s["Font"]
    if "OverSFX" in s:
        out["sfx_over"] = s["OverSFX"]
    if "PressSFX" in s:
        out["sfx_press"] = s["PressSFX"]
    if "ID" in s:
        out["id"] = s["ID"]
    color = s.get("color") or s.get("Color")
    if color is not None:
        out["color"] = f"#{color & 0xFFFFFFFF:08X}" if isinstance(color, int) else str(color)

    # Button state textures
    states: dict = {}
    # Element-style: NormalArt/GreyArt/PressArt/OverArt at idx 0/1/2/3
    for state_name, idx in (("NormalArt", 0), ("GreyArt", 1), ("PressArt", 2), ("OverArt", 3)):
        ts = _texture_state(s, state_name, idx)
        if ts:
            key = state_name.replace("Art", "").lower()
            states[key] = ts
    # Button-style: FileName + U/VNormal, U/VPressed, U/VHighlight, U/VDisabled
    if "FileName" in s and not states:
        art = s["FileName"]
        w, h = int(s.get("UWidth", 0)), int(s.get("VHeight", 0))
        for state_name, uk, vk in (
            ("normal",   "UNormal",   "VNormal"),
            ("pressed",  "UPressed",  "VPressed"),
            ("hover",    "UHighlight","VHighlight"),
            ("disabled", "UDisabled", "VDisabled"),
        ):
            if uk in s:
                states[state_name] = {
                    "art":         art,
                    "sprite_uv":   [int(s[uk]), int(s[vk])],
                    "sprite_size": [w, h],
                }
    if states:
        out["states"] = states

    # Animation color keyframes per state
    for state_name in ("Normal", "Pressed", "Highlight", "Disabled"):
        n_key = f"{state_name}AnimationTimeStamps"
        if n_key not in s:
            continue
        n = int(s[n_key])
        loops = bool(s.get(f"{state_name}AnimationLoops", False))
        keyframes = []
        for i in range(n):
            t = s.get(f"{state_name}Time{i}", 0.0)
            c = s.get(f"{state_name}Color{i}")
            kf: dict = {"time": float(t)}
            if c is not None:
                kf["color"] = f"#{c & 0xFFFFFFFF:08X}" if isinstance(c, int) else str(c)
            keyframes.append(kf)
        out.setdefault("state_animations", {})[state_name.lower()] = {
            "loops": loops, "keyframes": keyframes
        }

    return out


def _extract_static(s: dict) -> dict:
    out: dict = {
        "type": "static",
        "rect": _rect_from_section(s),
    }
    if "FileName" in s:
        out["art"] = s["FileName"]
        w, h = int(s.get("UWidth", 0)), int(s.get("VHeight", 0))
        u, v = int(s.get("UNormal", 0)), int(s.get("VNormal", 0))
        if w or h:
            out["sprite_uv"]   = [u, v]
            out["sprite_size"] = [w, h]
    if "Animation" in s:
        out["animation_in"]  = s["Animation"]
    if "AnimationOut" in s:
        out["animation_out"] = s["AnimationOut"]
    return out


def _extract_text(s: dict) -> dict:
    out: dict = {
        "type":   "text",
        "rect":   _rect_from_section(s),
    }
    for key in ("TextID", "Font", "Color", "color", "Alignment", "outline"):
        if key in s:
            out[key.lower()] = s[key]
    if "color" in out and isinstance(out["color"], int):
        out["color"] = f"#{out['color'] & 0xFFFFFFFF:08X}"
    if "Animation" in s:
        out["animation_in"]  = s["Animation"]
    if "AnimationOut" in s:
        out["animation_out"] = s["AnimationOut"]
    return out


def _extract_rect(s: dict) -> dict:
    out: dict = {
        "type":    "rect",
        "rect":    _rect_from_section(s),
    }
    color = s.get("Color") or s.get("color")
    if color is not None:
        out["color"] = f"#{color & 0xFFFFFFFF:08X}" if isinstance(color, int) else str(color)
    out["outline"] = bool(s.get("outline", False))
    if "Animation" in s:
        out["animation_in"]  = s["Animation"]
    return out


def _extract_meter(s: dict) -> dict:
    out: dict = {
        "type":   "meter",
        "rect":   _rect_from_section(s),
    }
    for key in ("NumberUnits", "UnitWidth", "unitHeight", "Skip",
                "ColorMin", "ColorMax", "Color"):
        if key in s:
            v = s[key]
            if isinstance(v, int) and key.startswith("Color"):
                v = f"#{v & 0xFFFFFFFF:08X}"
            out[key.lower()] = v
    return out


def _extract_anim_keyframes(s: dict) -> dict | None:
    """Extract an animation definition section."""
    n_key = "AnimationTimeStamps"
    if n_key not in s:
        return None
    n = int(s[n_key])
    loops = bool(s.get("AnimationLoops", False))
    keyframes = []
    for i in range(n):
        t = s.get(f"Time{i}", 0.0)
        kf: dict = {"time": float(t) if t != "xxx" else 0.0}
        px = s.get(f"Pos{i}X")
        py = s.get(f"Pos{i}Y")
        if px is not None:
            kf["offset"] = [int(px), int(py)]
        c = s.get(f"Color{i}")
        if c is not None:
            kf["color"] = f"#{c & 0xFFFFFFFF:08X}" if isinstance(c, int) else str(c)
        keyframes.append(kf)
    return {"loops": loops, "keyframes": keyframes}


# ---------------------------------------------------------------------------
# Section dispatcher
# ---------------------------------------------------------------------------

def _dispatch_element(section_name: str, s: dict) -> dict | None:
    """Guess element type and extract it from section data."""
    name_lower = section_name.lower()
    el_type = int(s.get("ElementType", -1))

    # Element-type explicit
    if el_type == 0:
        return _extract_background(s)
    if el_type == 1:
        return _extract_button(s)
    if el_type == 2:
        return _extract_text(s)
    if el_type == 3:
        return _extract_meter(s)

    # Name-based heuristic
    if any(x in name_lower for x in ("button", "scrollbutton", "scroll")):
        return _extract_button(s)
    if any(x in name_lower for x in ("rect",)):
        return _extract_rect(s)
    if any(x in name_lower for x in ("text", "label")):
        return _extract_text(s)

    # AnimObject — use type from data
    if "animobject" in name_lower:
        anim_type = int(s.get("type", -1))
        if anim_type == 1:  # text
            return _extract_text(s)
        if anim_type == 2:  # meter
            return _extract_meter(s)
        # default → static image
        return _extract_static(s)

    # Has FileName or Art0 → static
    if "FileName" in s or "Art0" in s:
        return _extract_static(s)

    # Has XLocation/Left + Width → unknown visual element
    if _rect_from_section(s):
        return {"type": "unknown", "rect": _rect_from_section(s), "raw": s}

    return None


# ---------------------------------------------------------------------------
# Main per-file conversion
# ---------------------------------------------------------------------------

NUMBERED_RE = re.compile(r"^([A-Za-z]+?)(\d+)$")


def convert_fit(fit_path: Path) -> dict:
    text = fit_path.read_text(encoding="utf-8", errors="replace")
    sections = parse_fit(text)

    elements: list[dict] = []
    animations: dict[str, dict] = {}
    named_elements: list[dict] = {}
    fonts: dict = {}
    meta: dict = {}

    # Count sections that look like group headers (e.g. [Buttons] → Buttoncount)
    group_headers: set[str] = set()
    for name, s in sections.items():
        for key in s:
            if key.lower().endswith("count") or key == "numElements":
                group_headers.add(name)

    for section_name, s in sections.items():
        # Skip group header-only sections
        if section_name in group_headers and not _rect_from_section(s):
            continue

        # Animation definition
        if ANIM_SECTION_RE.match(section_name):
            kf = _extract_anim_keyframes(s)
            if kf:
                animations[section_name] = kf
            continue

        # Fonts section
        if section_name.lower() == "fonts":
            fonts = {k: v for k, v in s.items()}
            continue

        # Elements / AnimObjects — numbered sequences
        m = NUMBERED_RE.match(section_name)
        if m:
            prefix = m.group(1)
            idx    = int(m.group(2))
            if any(prefix.lower().startswith(p.lower()) for p in ELEMENT_GROUP_PREFIXES):
                el = _dispatch_element(section_name, s)
                if el:
                    el["name"] = section_name
                    el["index"] = idx
                    elements.append(el)
                continue

        # Named layout sections (LoadingBar, WaitImage, etc.)
        if _rect_from_section(s):
            el = _dispatch_element(section_name, s)
            if el:
                el["name"] = section_name
                named_elements[section_name] = el

        # Top-level meta (canvas size, misc config)
        for key, val in s.items():
            if key.lower() in ("numtables", "groupcount"):
                continue
            meta[f"{section_name}.{key}"] = val

    # Sort elements by index
    elements.sort(key=lambda e: e.get("index", 0))

    # Infer canvas size from largest background element
    canvas = [640, 480]  # MC2 default
    for el in elements:
        if el.get("type") == "background" and el.get("rect"):
            r = el["rect"]
            w, h = r["left"] + r["width"], r["top"] + r["height"]
            if w * h > canvas[0] * canvas[1]:
                canvas = [w, h]

    out: dict = {
        "source":  fit_path.name,
        "canvas":  canvas,
        "note":    (
            f"Original MC2 layout at {canvas[0]}×{canvas[1]}px. "
            "In UMG: Canvas Panel with design resolution set to match. "
            "Use positions as pixel offsets."
        ),
    }
    if fonts:
        out["fonts"] = fonts
    if elements:
        out["elements"] = elements
    if named_elements:
        out["named_elements"] = named_elements
    if animations:
        out["animations"] = animations

    return out


# ---------------------------------------------------------------------------
# Batch runner
# ---------------------------------------------------------------------------

def run(art_dir: Path, out_dir: Path):
    fit_files = sorted(art_dir.glob("*.fit"))
    if not fit_files:
        print(f"No .fit files found in {art_dir}")
        return

    out_dir.mkdir(parents=True, exist_ok=True)
    ok = fail = 0

    for fit_path in fit_files:
        try:
            data = convert_fit(fit_path)
            out_path = out_dir / (fit_path.stem + ".json")
            out_path.write_text(json.dumps(data, indent=2), encoding="utf-8")
            n_el = len(data.get("elements", [])) + len(data.get("named_elements", {}))
            n_an = len(data.get("animations", {}))
            print(f"  OK  {fit_path.name:45s}  {n_el:3d} elements  {n_an:3d} animations")
            ok += 1
        except Exception as e:
            print(f"  FAIL {fit_path.name}: {e}")
            fail += 1

    print(f"\nDone: {ok} converted, {fail} failed → {out_dir}")


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    run(Path(sys.argv[1]), Path(sys.argv[2]))


if __name__ == "__main__":
    main()
