#!/usr/bin/env python3
"""Generates packed monochrome bitmap fonts for the e-paper UI from TTF files.

Ported from the original macOS-only CoreText/CoreGraphics implementation to
FreeType (via the freetype-py bindings), which runs on Linux (and anywhere
else FreeType is available) with no other dependency. The generated C++
output format is unchanged, so it's a drop-in replacement for the old
--output invocation documented in docs/asset-generation.md.

Covers the Latin-1 range 0x20-0xFC (32-252) instead of the old plain-ASCII
0x20-0x7E (32-126) range, so German umlauts (ä ö ü Ä Ö Ü ß, all of which
fall within that range) have real glyphs instead of falling back to '?'.
"""

from __future__ import annotations

import math
import sys
from dataclasses import dataclass
from pathlib import Path

import freetype


@dataclass(frozen=True)
class FontSpec:
    path: str
    symbol: str
    size: float


@dataclass(frozen=True)
class GlyphRecord:
    width: int
    height: int
    bearing_x: int
    bearing_y: int
    advance: int
    bitmap: list[int]


class FontGenError(RuntimeError):
    pass


# Latin-1 covers German umlauts (Ä=0xC4, Ö=0xD6, Ü=0xDC, ß=0xDF, ä=0xE4,
# ö=0xF6, ü=0xFC) contiguously alongside plain ASCII, so a single contiguous
# glyph range keeps the lookup in bitmap_font.cpp a simple bounds-checked
# array index -- no need for a sparse codepoint table.
kCharFirst = 0x20
kCharLast = 0xFC


def parse_args(argv: list[str]) -> tuple[Path, list[FontSpec]]:
    if len(argv) < 4 or argv[1] != "--output":
        raise FontGenError(
            "usage: generate_epaper_fonts.py --output <file> <font-path:symbol:size>..."
        )

    output_path = Path(argv[2])
    specs: list[FontSpec] = []
    for raw in argv[3:]:
        parts = raw.split(":", 2)
        if len(parts) != 3:
            raise FontGenError(f"invalid font spec: {raw}")
        try:
            size = float(parts[2])
        except ValueError as exc:
            raise FontGenError(f"invalid font size in spec: {raw}") from exc
        specs.append(FontSpec(path=parts[0], symbol=parts[1], size=size))

    return output_path, specs


def load_face(spec: FontSpec) -> freetype.Face:
    try:
        face = freetype.Face(spec.path)
    except freetype.FT_Exception as exc:
        raise FontGenError(f"unable to load font: {spec.path} ({exc})") from exc
    # Pixel size directly, matching the original CoreText usage where the
    # requested "size" was treated 1:1 as pixels in an unscaled bitmap
    # context (no separate point-to-pixel DPI conversion).
    face.set_pixel_sizes(0, int(round(spec.size)))
    return face


def unpack_mono_bitmap(bitmap: "freetype.Bitmap") -> list[int]:
    """Expands FreeType's byte-padded 1bpp rows into one 0/255 byte per pixel."""
    width = bitmap.width
    rows = bitmap.rows
    pitch = bitmap.pitch  # bytes per row, includes right-edge padding
    buf = bitmap.buffer

    samples = [0] * (width * rows)
    for row in range(rows):
        row_start = row * pitch
        for col in range(width):
            byte_index = row_start + (col // 8)
            bit_mask = 0x80 >> (col % 8)
            bit_set = (buf[byte_index] & bit_mask) != 0
            samples[row * width + col] = 0 if bit_set else 255
    return samples


def pack_bitmap(samples: list[int], width: int, height: int) -> list[int]:
    """Repacks per-pixel samples tightly (no per-row padding), MSB first."""
    packed: list[int] = []
    current_byte = 0
    bit_index = 0

    for row in range(height):
        for col in range(width):
            sample = samples[row * width + col]
            if sample < 128:
                current_byte |= 0x80 >> bit_index
            bit_index += 1
            if bit_index == 8:
                packed.append(current_byte)
                current_byte = 0
                bit_index = 0

    if bit_index:
        packed.append(current_byte)
    return packed


def render_glyph(face: freetype.Face, character: int) -> GlyphRecord:
    load_flags = freetype.FT_LOAD_RENDER | freetype.FT_LOAD_TARGET_MONO
    try:
        face.load_char(chr(character), load_flags)
    except freetype.FT_Exception:
        # Missing glyph in this font: fall back to '?', which every one of
        # our source fonts (Inter) is guaranteed to have.
        if character == ord("?"):
            raise FontGenError("font is missing fallback glyph '?'") from None
        return render_glyph(face, ord("?"))

    glyph = face.glyph
    bitmap = glyph.bitmap
    advance = max(1, math.ceil(glyph.advance.x / 64.0))

    width = bitmap.width
    height = bitmap.rows
    bearing_x = glyph.bitmap_left
    bearing_y = glyph.bitmap_top

    if width == 0 or height == 0:
        return GlyphRecord(
            width=0,
            height=0,
            bearing_x=bearing_x,
            bearing_y=bearing_y,
            advance=advance,
            bitmap=[],
        )

    samples = unpack_mono_bitmap(bitmap)
    packed = pack_bitmap(samples, width, height)
    return GlyphRecord(
        width=width,
        height=height,
        bearing_x=bearing_x,
        bearing_y=bearing_y,
        advance=advance,
        bitmap=packed,
    )


def emit_byte_array(values: list[int], indent: str) -> str:
    if not values:
        return ""

    lines: list[str] = []
    line = indent
    for index, value in enumerate(values):
        entry = f"0x{value:02X}"
        if index == 0:
            line += entry
        elif len(line) + len(entry) + 2 > 100:
            lines.append(line + ",")
            line = indent + entry
        else:
            line += ", " + entry
    lines.append(line)
    return "\n".join(lines)


def generate_source(specs: list[FontSpec]) -> str:
    out = [
        '#include "epaper_ui/generated_epaper_fonts.h"',
        "",
        "namespace epaper_fonts {",
        "",
    ]

    for spec in specs:
        face = load_face(spec)
        metrics = face.size
        # FT_Size_Metrics fields are always 26.6 fixed-point (1/64 px),
        # regardless of how the pixel size was requested.
        ascent = max(1, math.ceil(metrics.ascender / 64.0))
        descent = max(0, math.ceil(-metrics.descender / 64.0))
        raw_line_height = max(1, math.ceil(metrics.height / 64.0))
        # FreeType's "height" already folds in the recommended line gap;
        # derive it explicitly so line_height = ascent + descent + leading
        # matches the original CoreText-based formula.
        leading = max(0, raw_line_height - (ascent + descent))
        line_height = max(1, ascent + descent + leading)

        all_bitmap_bytes: list[int] = []
        glyph_lines: list[str] = []
        for code in range(kCharFirst, kCharLast + 1):
            glyph = render_glyph(face, code)
            offset = len(all_bitmap_bytes)
            all_bitmap_bytes.extend(glyph.bitmap)
            glyph_lines.append(
                "    { %5u, %4u, %3u, %3u, %4d, %4d, %3u },"
                % (
                    offset,
                    len(glyph.bitmap),
                    glyph.width,
                    glyph.height,
                    glyph.bearing_x,
                    glyph.bearing_y,
                    glyph.advance,
                )
            )

        out.extend(
            [
                "namespace {",
                f"const epaper_font::GlyphBitmap {spec.symbol}_glyphs[] = {{",
                *glyph_lines,
                "};",
                "",
                f"const uint8_t {spec.symbol}_bitmaps[] = {{",
            ]
        )
        if all_bitmap_bytes:
            out.append(emit_byte_array(all_bitmap_bytes, indent="    "))
        out.extend(
            [
                "};",
                "}  // namespace",
                "",
                f"const epaper_font::BitmapFont {spec.symbol} = {{",
                f'    "{spec.symbol}",',
                f"    {kCharFirst},",
                f"    {kCharLast},",
                f"    {line_height},",
                f"    {ascent},",
                f"    {spec.symbol}_glyphs,",
                f"    {spec.symbol}_bitmaps,",
                "};",
                "",
            ]
        )

    out.append("}  // namespace epaper_fonts")
    out.append("")
    return "\n".join(out)


def main(argv: list[str]) -> int:
    try:
        output_path, specs = parse_args(argv)
        source = generate_source(specs)
        output_path.write_text(source, encoding="utf-8")
        return 0
    except FontGenError as exc:
        sys.stderr.write(f"font generation failed: {exc}\n")
        return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
