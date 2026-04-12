"""
Convert font files (.ttf, .bdf and others supported by freetype) to the internally used bitmap format (.fnt file).

Needs freetype-py installed.
"""
from enum import Flag
import re
import argparse
from pathlib import Path
from struct import pack, calcsize
import struct
from PIL import Image
from glob import glob
import freetype as ft

# typedef struct {
#     uint8_t width;  // bitmap width [pixels]
#     uint8_t height;  // bitmap height [pixels]
#     int8_t lsb;  // left side bearing
#     int8_t tsb;  // top side bearing
#     int8_t advance;  // cursor advance width
#     uint32_t start_index;  // offset to first byte of bitmap data (relative to start of glyph description table)
# } glyph_description_t;
FMT_GLYPH_DESCRIPTION = "BBbbbI"

# typedef struct {
#     uint16_t n_glyphs;  // number of glyphs in this font file
#     // simple ascci mapping parameters
#     uint16_t ascii_map_start = 32;  // the first glyph maps to this codepoint
#     uint16_t ascii_map_n = 96;  // how many glyphs map to incremental codepoints
#     // offset to optional glyph id to codepoint mapping table
#     uint16_t map_table_offset = glyph_description_offset;
#     uint32_t glyph_description_offset;
#     uint32_t glyph_data_offset;
#     uint16_t linespace;
#     int8_t yshift;  // to vertically center the digits, add this to tsb
#     // bit0: has_outline. glyph index of outline = glyph index of fill * 2
#     uint8_t flags;
#     const char[] name; // length = map_table_offset - sizeof(font_header_t)
# } font_header_t;
FMT_HEADER = "IHHHHIIHbB"


class FLAGS(Flag):
    # Flag is set if outline glyphs are available in the font (doubles the number of glyphs)
    HAS_OUTLINE = 1 << 0

    # Pixel format_B/_A: 00 = 1 bit, 01 = 8 bit monochrome, 10 = 4 bit monochrome
    PIX_FORMAT_A = 1 << 1
    PIX_FORMAT_B = 1 << 2

    # In monospace-mode all glyphs have the same size and avance_width.
    # If this is given, the glyph_dsc_t table only has a single entry.
    MONOSPACE = 1 << 3


def get_next_filename(out_dir, file_ending=".fnt"):
    fnt_files = glob(str(out_dir / "*" + file_ending))

    for i in range(1000):
        fname = out_dir / f"{i:03d}{file_ending}"
        if str(fname) not in fnt_files:
            return fname


def get_all_cps(face):
    all_cps = []
    all_cps.append(face.get_first_char()[0])

    while True:
        all_cps.append(face.get_next_char(all_cps[-1], 0)[0])
        if all_cps[-1] == 0:
            break
    return all_cps


def get_cp_set(args, face):
    """assemble the set of code-points to convert"""
    cp_set = set()

    if args.add_all:
        cp_set.update(get_all_cps(face))

    if args.add_range is not None:
        cp_set.update((int(x, base=0) for x in args.add_range.split(",")))

    if args.add_ascii:
        cp_set.update(range(0x20, 0x20 + 95))

    if args.add_numerals:
        cp_set.update([ord(x) for x in "1234567890:"])

    # We don't support ascii codes below 0x20
    cp_set.difference_update(range(0x20))

    return sorted(cp_set)


def get_n_ascii(cp_set):
    """how many characters correspond to the basic ascii scheme?"""
    first_char = 0
    for i, cp in enumerate(cp_set):
        if i == 0:
            first_char = cp
        elif first_char + i != cp:
            return (first_char, i)
    return (first_char, len(cp_set))


import freetype as ft


def get_monospace_metrics(face: ft.Face, outline_radius=0):
    """Calculates fixed cell dimensions and origin offsets for monospace rendering."""
    if face.is_scalable:
        ascender = face.size.ascender >> 6
        descender = face.size.descender >> 6
        max_advance = face.size.max_advance >> 6
    else:
        ascender = face.ascender
        descender = face.descender
        max_advance = face.max_advance_width

        if ascender == 0 and descender == 0:
            ascender = face.bbox.yMax
            descender = face.bbox.yMin
        if max_advance == 0:
            max_advance = face.bbox.xMax - face.bbox.xMin

        if max_advance == 0 and face.num_fixed_sizes > 0:
            max_advance = face.available_sizes[0].width
            if ascender == 0 and descender == 0:
                ascender = face.available_sizes[0].height
                descender = 0

    cell_width = max_advance + (2 * outline_radius)
    cell_height = ascender - descender + (2 * outline_radius)
    origin_x = outline_radius
    origin_y = ascender + outline_radius

    return {
        "cell_width": cell_width,
        "cell_height": cell_height,
        "origin_x": origin_x,
        "origin_y": origin_y,
    }


def get_glyph(code: int, face: ft.Face, outline_radius=0, monospace_metrics=None):
    """returns a rendered bitmap of a glyph and its metadata
    the bitmap is in 1 bit / pixel or 8 bit / pixel, depending if a true type font or
    a bitmap font was rendered.
    """
    face.select_charmap(ft.FT_ENCODING_UNICODE)

    if outline_radius > 0:
        stroker = ft.Stroker()
        stroker.set(
            outline_radius, ft.FT_STROKER_LINECAP_ROUND, ft.FT_STROKER_LINEJOIN_ROUND, 0
        )
        face.load_char(code, ft.FT_LOAD_DEFAULT | ft.FT_LOAD_NO_BITMAP)
        glyph = face.glyph.get_glyph()
        glyph.stroke(stroker, True)
    else:
        face.load_char(code, ft.FT_LOAD_DEFAULT)
        glyph = face.glyph.get_glyph()

    # these values are not valid for outline mode
    metrics = face.glyph.metrics

    blyph = glyph.to_bitmap(ft.FT_RENDER_MODE_NORMAL, ft.Vector(0, 0), True)
    bitmap = blyph.bitmap

    if bitmap.pixel_mode not in (ft.FT_PIXEL_MODE_MONO, ft.FT_PIXEL_MODE_GRAY):
        raise NotImplementedError(f"pixel_mode not supported yet: {bitmap.pixel_mode}")

    source_buffer = bytes(bitmap.buffer)
    source_pitch = bitmap.pitch
    source_width = bitmap.width
    source_height = bitmap.rows

    if monospace_metrics:
        cell_width = monospace_metrics["cell_width"]
        cell_height = monospace_metrics["cell_height"]
        origin_x = monospace_metrics["origin_x"]
        origin_y = monospace_metrics["origin_y"]

        # Calculate the destination coordinates for the source bitmap
        target_x = origin_x + blyph.left
        target_y = origin_y - blyph.top

        if bitmap.pixel_mode == ft.FT_PIXEL_MODE_GRAY:
            target_pitch = cell_width
            target_buffer = bytearray(target_pitch * cell_height)

            for row in range(source_height):
                sy = row
                dy = target_y + sy
                if 0 <= dy < cell_height:
                    for col in range(source_width):
                        sx = col
                        dx = target_x + sx
                        if 0 <= dx < cell_width:
                            target_buffer[dy * target_pitch + dx] = source_buffer[
                                sy * source_pitch + sx
                            ]

        elif bitmap.pixel_mode == ft.FT_PIXEL_MODE_MONO:
            # Monochrome pitch requires rounding up to the nearest byte
            target_pitch = (cell_width + 7) // 8
            target_buffer = bytearray(target_pitch * cell_height)

            for row in range(source_height):
                sy = row
                dy = target_y + sy
                if 0 <= dy < cell_height:
                    for col in range(source_width):
                        sx = col
                        dx = target_x + sx
                        if 0 <= dx < cell_width:
                            # Extract specific bit from the source
                            src_byte = source_buffer[sy * source_pitch + (sx // 8)]
                            src_bit = (src_byte >> (7 - (sx % 8))) & 1
                            if src_bit:
                                # Apply the bit to the correct position in the target
                                target_buffer[dy * target_pitch + (dx // 8)] |= 1 << (
                                    7 - (dx % 8)
                                )

        final_buffer = bytes(target_buffer)
        final_pitch = target_pitch
        final_width = cell_width
        final_height = cell_height
        final_lsb = 0
        final_tsb = 0

    else:
        final_buffer = source_buffer
        final_pitch = source_pitch
        final_width = source_width
        final_height = source_height
        final_lsb = blyph.left
        final_tsb = blyph.top

    props = {
        # number of bytes taken per row
        "pitch": final_pitch,
        # 1 = A monochrome bitmap, using 1 bit per pixel, MSB first,  2 = Each pixel is stored in one byte
        "pixel_mode": bitmap.pixel_mode,
        "width": final_width,
        "height": final_height,
        "lsb": final_lsb,
        "tsb": final_tsb,
        "advance_hr": face.glyph.advance.x,
    }

    if outline_radius == 0:
        # these metrics are only valid if the Stroker() is not used
        props["height_hr"] = metrics.height
        props["lsb_hr"] = metrics.horiBearingX
        props["tsb_hr"] = metrics.horiBearingY

    return final_buffer, props


def get_y_stats(glyph_props, DISPLAY_HEIGHT=32):
    """
    for the given list of glyph properties, returns their
      * maximum bounding box height (need to divide by 64)
      * y-shift needed to center the bounding box (need to divide by 64)
    """
    us = []
    ls = []
    for p in glyph_props:
        y_max = p["tsb_hr"]
        y_min = p["tsb_hr"] - p["height"] * 64
        # print(f'{y_max:3d}  {y_min:3d}')
        us.append(y_max)
        ls.append(y_min)

    bb_up = max(us)
    bb_down = min(ls)
    bb_mid = (bb_up + bb_down) / 2

    bb_height = bb_up - bb_down
    yshift = round(DISPLAY_HEIGHT * 64 / 2 - bb_mid)

    return bb_height, yshift


def auto_tune_font_size(face, args):
    """
    Find the correct char_size to fill the whole height of the display
    returns
      * char_size for face.set_char_size(height=char_size)
      * fractional yshift (needs to be divided by 64)
    """
    print("\nTuning font size ...")
    char_size = int(args.font_height * 64)
    yshift = 0

    for i in range(16):
        face.set_char_size(height=char_size)
        props = [get_glyph(c, face)[1] for c in args.test_string]
        bb_height, yshift = get_y_stats(props)
        print(
            f"    height: {char_size / 64:4.1f} --> {bb_height / 64:4.1f}, {yshift / 64:.1f}"
        )
        err = args.font_height * 64 - bb_height
        if err == 0:
            print("    👍")
            break
        char_size += round(err / 2)

    # Make sure the width of the digits (88:88) fits on the display
    for i in range(8):
        advs = [p["advance_hr"] for p in props]
        w_clock = 4 * max(advs[:-1]) + advs[-1]
        margin = args.max_width * 64 - w_clock
        print(f"    width:  {char_size / 64:4.1f} --> {w_clock / 64:.1f}")
        if margin > 0:
            print("    👍")
            break

        char_size += margin // 4
        face.set_char_size(height=char_size)
        props = [get_glyph(c, face)[1] for c in args.test_string]
        bb_height, yshift = get_y_stats(props)

    return char_size, yshift


def print_table(vals, w=24, w_v=3, f=None):
    ll = len(vals) - 1
    for i, g in enumerate(vals):
        if i > 0 and (i % w) == 0:
            print(file=f)
        print(f"{g:{w_v}d}", end="", file=f)
        if i < ll:
            print(",", end="", file=f)
    print("\n};\n", file=f)


def eight_to_four(buf):
    """Pack two pixels in one output byte"""
    return bytes(
        (buf[i * 2] & 0xF0) | (buf[i * 2 + 1] >> 4) for i in range(len(buf) // 2)
    )


def convert(args, face: ft.Face, yshift: int, outline_radius=0):
    """
    If outline_radius is > 0 it will add the normal glyphs to the font and in
    addition the outline glyphs with the set stroke width.
    """
    cp_set = get_cp_set(args, face)
    if len(cp_set) == 0:
        raise RuntimeError("No glyphs selected to convert")

    outlines = [0]
    if outline_radius > 0:
        # If we want outlines, we export all glyphs twice!
        outlines.append(outline_radius)

    # --------------------------------------------------
    #  Generate the glyph bitmap data
    # --------------------------------------------------
    glyph_data = []
    glyph_props = []

    for ol_radius in outlines:
        if ol_radius == 0:
            print("    * exporting normal glyphs")
        else:
            print(f"    * exporting outline glyphs. Radius: {ol_radius / 32:.1f} px")

        if args.monospace:
            monospace_metrics = get_monospace_metrics(face, outline_radius=ol_radius)
            print("monospace_metrics:", monospace_metrics)
        else:
            monospace_metrics = None

        for code in cp_set:
            buf, props = get_glyph(
                chr(code),
                face,
                outline_radius=ol_radius,
                monospace_metrics=monospace_metrics,
            )
            if props["pixel_mode"] == ft.FT_PIXEL_MODE_GRAY and args.four_bpp:
                buf = eight_to_four(buf)
            props["cp"] = code
            glyph_props.append(props)
            glyph_data.append(buf)

    # --------------------------------------------------
    #  Generate the codepoint to glyph mapping table
    # --------------------------------------------------
    # convert cp_set to uint32_t
    ascii_map_start, ascii_map_n = get_n_ascii(cp_set)
    map_table = cp_set[ascii_map_n:]
    print(f"    ascii_map_start: 0x{ascii_map_start:02x}, ascii_map_n: {ascii_map_n}")
    print(f"    map_table: {len(map_table)} 32 bit words")

    # --------------------------------------------------
    #  Collect header info
    # --------------------------------------------------
    # Get the line-height from the numeral bounding box / the OS/2 table
    ls = face.size.height // 64

    if ls == 0:
        print("WARNING!! Setting linespace to 8")
        ls = 8

    # Set the flags bits
    flags = FLAGS(0)
    if outline_radius > 0:
        flags |= FLAGS.HAS_OUTLINE
    if args.monospace:
        flags |= FLAGS.MONOSPACE
    pix_mode = glyph_props[0]["pixel_mode"]
    if pix_mode == ft.FT_PIXEL_MODE_MONO:
        pass
    elif pix_mode == ft.FT_PIXEL_MODE_GRAY:
        if args.four_bpp:
            flags |= FLAGS.PIX_FORMAT_B
        else:
            flags |= FLAGS.PIX_FORMAT_A
    else:
        RuntimeError(
            "pixel mode of this font file is not supported. Can only do 1, 4 or 8 bpp.",
            pix_mode,
        )

    header = {
        "name": face.family_name.decode(),
        "linespace": ls,
        "yshift": yshift,
        "flags": flags,
        "ascii_map_start": ascii_map_start,
        "ascii_map_n": ascii_map_n,
    }

    return glyph_props, glyph_data, map_table, header


def export_as_fnt(
    glyph_props: list[dict],
    glyph_data: list[bytes],
    map_table: list[int],
    header: dict,
    out_name: str,
):
    # --------------------------------------------------
    #  Generate the glyph description table
    # --------------------------------------------------
    glyph_description_bs = bytes()
    glyph_data_bs = bytes()
    for i, (props, data) in enumerate(zip(glyph_props, glyph_data)):
        props["start_index"] = len(glyph_data_bs)
        bs = pack(
            FMT_GLYPH_DESCRIPTION,
            props["width"],
            props["height"],
            props["lsb"],
            props["tsb"],
            props["advance_hr"] // 64,
            props["start_index"],
        )
        # Produce only a single entry in monospace mode. All glyphs are the same.
        if i == 0 or FLAGS.MONOSPACE not in header["flags"]:
            glyph_description_bs += bs
        glyph_data_bs += data
        # TODO handle StructError for some glyphs
    print("    glyph_data:", len(glyph_data_bs), "bytes")
    print("    glyph_desc:", len(glyph_description_bs), "bytes")

    # map_table: Convert int to 4 bytes
    map_table_bs = b"".join([cp.to_bytes(4, signed=False) for cp in map_table])

    # --------------------------------------------------
    #  Generate the font description header
    # --------------------------------------------------
    map_table_offset = calcsize(FMT_HEADER) + len(header["name"]) + 1
    glyph_description_offset = map_table_offset + len(map_table_bs)
    glyph_data_offset = glyph_description_offset + len(glyph_description_bs)

    font_header_bs = pack(
        FMT_HEADER,
        0x005A54BE,  # magic number
        len(glyph_props),
        header["ascii_map_start"],
        header["ascii_map_n"],
        map_table_offset,
        glyph_description_offset,
        glyph_data_offset,
        header["linespace"],
        header["yshift"] // 64,
        header["flags"].value,
    )

    all_bs = (
        font_header_bs
        + header["name"].encode("ascii")
        + b"\x00"
        + map_table_bs
        + glyph_description_bs
        + glyph_data_bs
    )

    with open(out_name, "wb") as f:
        f.write(all_bs)

    print(f"    wrote {out_name} ({len(all_bs) / 1024:.1f} kB)")


def export_as_header(
    glyph_props: list[dict],
    glyph_data: list[bytes],
    map_table: list[int],
    header: dict,
    out_name: str,
):
    # --------------------------------------------------
    #  Generate the glyph description table
    # --------------------------------------------------
    glyph_data_bs = bytes()
    for props, data in zip(glyph_props, glyph_data):
        props["start_index"] = len(glyph_data_bs)
        glyph_data_bs += data
    print("    glyph_data:", len(glyph_data_bs), "bytes")

    name = header["name"]

    with open(out_name, "w") as f:
        print(
            f"""\
#include <stdint.h>
#include <stdio.h>
#include <font_draw.h>
// -----------------------------------
//  {name}
// -----------------------------------

static const uint8_t glyphs_{name}[{len(glyph_data_bs)}] = {{""",
            file=f,
        )
        print_table(glyph_data_bs, f=f)

        print(
            f"""\
// GLYPH DESCRIPTION
static const glyph_dsc_t glyph_dsc_{name}[{len(glyph_props)}] = {{""",
            file=f,
        )
        for props in glyph_props:
            cp = props.pop("cp")
            props["advance"] = props["advance_hr"] // 64
            print("    {", end="", file=f)
            for k in ("advance", "width", "height", "lsb", "tsb", "start_index"):
                print(f".{k} = {props[k]:2d}, ", end="", file=f)
            print(f"}},  // U+{cp:04X} '{chr(cp)}'", file=f)
            # Produce only a single entry in monospace mode. All glyphs are the same.
            if FLAGS.MONOSPACE in header["flags"]:
                break
        print("};\n", file=f)

        cp_table_name = "NULL"
        if len(map_table) > 0:
            cp_table_name = f"code_points_{name}"
            print(
                f"static const unsigned {cp_table_name}[{len(map_table)}] = {{",
                file=f,
            )
            print_table(map_table, w=19, w_v=6, f=f)

        flag_str = " | ".join([f.name for f in header["flags"]])
        print(
            f"""\
const font_t f_{name} = {{
    .magic = 0x005A54BE,
    .linespace = {header["linespace"]},
    .n_glyphs = {len(glyph_props)},
    .map_start = {header['ascii_map_start']},
    .map_n = {header['ascii_map_n']},
    .map_table = {cp_table_name},
    .glyph_description_table = glyph_dsc_{name};
    .glyph_data_table = glyphs_{name};
    .flags = {flag_str},
    .name = \"{header['name']}\"
}};
        """,
            file=f,
        )

    print(
        f"wrote {out_name} ({len(glyph_data_bs) / 1024:.1f} kB)",
    )


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument(
        "--font-height",
        default=30.0,
        type=float,
        help="Target height of the digits. Only used for vector inputs. default = 30 pixels",
    )
    parser.add_argument(
        "--tune-size",
        action="store_true",
        help="Tune the font-size such that they look good on the Espirgbani LED clock.",
    )
    parser.add_argument(
        "--max-width",
        default=128,
        help="Maximum width of 4 times the widest digit. Default: 128 pixels. Only used with --tune-size.",
    )
    parser.add_argument(
        "--test-string",
        default="0123456789:",
        help="Test string to use to tune the target digit height. Only used with --tune-size.",
    )
    parser.add_argument(
        "--outline-radius",
        default=0,
        type=int,
        help="If > 0, will add the outline glyphs. Doubles the number of glyphs in the font. [pixels / 64], default = 0.",
    )
    parser.add_argument(
        "--add-numerals",
        action="store_true",
        help="Add 0-9 and : to the generated file",
    )
    parser.add_argument(
        "--add-ascii",
        action="store_true",
        help="Add 95 Ascii characters to the generated file",
    )
    parser.add_argument(
        "--add-all", action="store_true", help="Add all glyphs in the font"
    )
    parser.add_argument(
        "--add-range", help="Add comma separated list of unicodes to the generated file"
    )

    parser.add_argument(
        "fontfile", help="Name of the .ttf, .otf, .woff2 or .bdf file to convert"
    )
    parser.add_argument(
        "--numeric-name",
        action="store_true",
        help="Use a numerical file name, which is incrementing with every file",
    )
    parser.add_argument(
        "--fnt",
        action="store_true",
        help="Write a binary .fnt file instead instead of a .h",
    )
    parser.add_argument(
        "--four_bpp",
        action="store_true",
        help="Convert 8-bit greyscale to 4-bit greyscale",
    )
    parser.add_argument(
        "--monospace",
        action="store_true",
        help="In monospace mode, all glyphs are forced to the same properties",
    )

    args = parser.parse_args()

    # Load an existing font file
    face = ft.Face(args.fontfile)

    # determine its optimum size
    yshift = 0
    if face.is_scalable:
        char_size = int(args.font_height * 64)
        if args.tune_size:
            char_size, yshift = auto_tune_font_size(face, args)
        face.set_char_size(height=char_size)

    # generate bitmap font
    glyph_props, glyph_data, map_table, header = convert(
        args, face, yshift, args.outline_radius
    )

    # Generate the output file name
    out_name = Path("fnt/")
    out_name.mkdir(exist_ok=True)
    f_ending = ".fnt" if args.fnt else ".h"
    if args.numeric_name:
        out_name = get_next_filename(out_name, f_ending)
    else:
        tmp = re.sub("[^A-Za-z0-9]+", "_", face.family_name.decode().lower())
        out_name = (out_name / tmp).with_suffix(f_ending)
    out_name_png = out_name.with_suffix(".png")
    # if out_name.is_file():
    #     print("file exists")
    #     exit(0)

    if args.fnt:
        print(f"\nGenerating binary .fnt file ...")
        export_as_fnt(glyph_props, glyph_data, map_table, header, str(out_name))
    else:
        print(f"\nGenerating C-header .h file ...")
        export_as_header(glyph_props, glyph_data, map_table, header, str(out_name))


if __name__ == "__main__":
    main()
