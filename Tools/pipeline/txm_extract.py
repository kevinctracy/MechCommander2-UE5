#!/usr/bin/env python3
"""
MC2 TXM Texture Extractor
Converts MechCommander 2 .txm files to PNG.

TXM Format (from txmmgr.cpp + lzdecomp.cpp):
  On-disk .txm files are custom LZW-compressed data (9-12 bit variable codes,
  matching MC2's lzdecomp.cpp algorithm). After decompression the payload is a
  raw TGA file that GOS loads via gos_NewTextureFromMemory.

  The LZW variant:
    - Code width starts at BASE_BITS=9, grows to 12
    - Code 256 = HASH_CLEAR (reset dictionary)
    - Code 257 = HASH_EOF  (end of data)
    - Code 258 = first dynamic entry
    - Dictionary entry: (chain: uint16, suffix: uint8) — exactly LZW

Usage:
    pip install Pillow
    python txm_extract.py <input_dir> <output_dir>
    python txm_extract.py FinalBuild/data/textures/ extracted_textures/

Note: The 2315 source .tga files in Source/Data/ cover most textures and can be
imported directly to UE5 without extraction. Use this script only for the 103
runtime .txm files in FinalBuild/data/textures/ that may not have source equivalents.
"""

import io
import sys
import struct
from pathlib import Path

HASH_CLEAR = 256
HASH_EOF   = 257
HASH_FREE  = 258
BASE_BITS  = 9
MAX_BITS   = 12


def lzw_decompress(data: bytes) -> bytes:
    """
    MC2 LZW decompressor matching lzdecomp.cpp.
    Reads variable-width codes (9→12 bits) from a little-endian bitstream.
    """
    out = bytearray()
    bit_buf = 0
    bit_cnt = 0
    pos = 0

    # Dictionary: index -> bytes string (built from suffix + chain)
    # Entries 0-255 = single bytes, 256/257 = control codes, 258+ = dynamic
    table = [bytes([i]) for i in range(256)]  # 0-255
    table.append(b"")  # 256 = CLEAR placeholder
    table.append(b"")  # 257 = EOF placeholder

    code_size = BASE_BITS
    max_code = (1 << code_size)

    def read_code():
        nonlocal bit_buf, bit_cnt, pos
        while bit_cnt < code_size and pos < len(data):
            bit_buf |= data[pos] << bit_cnt
            bit_cnt += 8
            pos += 1
        if bit_cnt < code_size:
            return HASH_EOF
        code = bit_buf & ((1 << code_size) - 1)
        bit_buf >>= code_size
        bit_cnt -= code_size
        return code

    prev = None

    while True:
        code = read_code()
        if code == HASH_EOF:
            break
        if code == HASH_CLEAR:
            table = [bytes([i]) for i in range(256)]
            table.append(b"")
            table.append(b"")
            code_size = BASE_BITS
            max_code = 1 << code_size
            prev = None
            continue

        if code < len(table):
            entry = table[code]
        elif code == len(table) and prev is not None:
            # Special LZW case: code not yet in table
            entry = prev + prev[:1]
        else:
            raise ValueError(f"Bad LZW code {code} (table size {len(table)})")

        out.extend(entry)

        if prev is not None:
            new_entry = prev + entry[:1]
            table.append(new_entry)
            # Grow code width when we fill the current range
            if len(table) >= max_code and code_size < MAX_BITS:
                code_size += 1
                max_code = 1 << code_size

        prev = entry

    return bytes(out)


def bgra_to_image(data: bytes) -> tuple:
    """
    Convert raw BGRA pixel data (GOS/DirectX format) to a PIL Image.
    After LZW decompression, TXM payload is always raw 64x64 BGRA 32bpp.
    Returns (PIL Image, has_alpha: bool).
    """
    try:
        from PIL import Image
    except ImportError:
        raise RuntimeError("Pillow not installed. Run: pip install Pillow")

    num_pixels = len(data) // 4
    import math
    side = int(math.isqrt(num_pixels))
    if side * side != num_pixels:
        raise ValueError(f"Non-square pixel count {num_pixels}")

    # BGRA -> RGBA swap
    rgba = bytearray(len(data))
    for i in range(num_pixels):
        b = data[i*4]
        g = data[i*4+1]
        r = data[i*4+2]
        a = data[i*4+3]
        rgba[i*4]   = r
        rgba[i*4+1] = g
        rgba[i*4+2] = b
        rgba[i*4+3] = a

    has_alpha = any(rgba[i*4+3] < 255 for i in range(min(num_pixels, 256)))
    img = Image.frombytes("RGBA", (side, side), bytes(rgba))
    return img, has_alpha


def extract_txm(src_path: Path, dst_path: Path) -> tuple:
    """Returns (success: bool, has_alpha: bool)."""
    raw = src_path.read_bytes()
    try:
        decompressed = lzw_decompress(raw)
    except Exception as e:
        print(f"\n    LZW error: {e}")
        return False, False

    try:
        img, has_alpha = bgra_to_image(decompressed)
    except Exception as e:
        print(f"\n    Pixel decode error: {e}")
        return False, False

    dst_path.parent.mkdir(parents=True, exist_ok=True)
    img.save(dst_path)
    return True, has_alpha


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    input_dir  = Path(sys.argv[1])
    output_dir = Path(sys.argv[2])
    output_dir.mkdir(parents=True, exist_ok=True)

    txm_files = sorted(input_dir.glob("**/*.txm"))
    if not txm_files:
        print(f"No .txm files found in {input_dir}")
        sys.exit(1)

    print(f"Found {len(txm_files)} .txm files")
    ok = skip = 0
    manifest_rows = []

    for txm in txm_files:
        rel = txm.relative_to(input_dir)
        dst = output_dir / rel.with_suffix(".png")
        print(f"  {txm.name} -> {dst.name} ", end="", flush=True)
        success, has_alpha = extract_txm(txm, dst)
        if success:
            compression = "BC3" if has_alpha else "BC1"
            manifest_rows.append(f"{txm.name},{dst.name},{compression}")
            print(f"[{compression}]")
            ok += 1
        else:
            print("[SKIP]")
            skip += 1

    manifest_path = output_dir / "ue_import_manifest.csv"
    with open(manifest_path, "w") as f:
        f.write("SourceFile,OutputName,UECompression\n")
        f.write("\n".join(manifest_rows))

    print(f"\nDone: {ok} extracted, {skip} skipped")
    print(f"Import manifest written to: {manifest_path}")


if __name__ == "__main__":
    main()
