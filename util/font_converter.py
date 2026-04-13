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
from sys import flags
from PIL import Image, ImageDraw
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


def get_next_filename(out_dir: Path, file_ending=".fnt"):
    """expects files with 3 numerical digits in out_dir. Returns the next free name"""
    fnt_files = glob(str((out_dir / "*").with_suffix(file_ending)))

    for i in range(1000):
        fname = out_dir / f"{i:03d}{file_ending}"
        if str(fname) not in fnt_files:
            return fname


def get_all_cps(face: ft.Face):
    """return all supported code-points of the font"""
    all_cps = []
    all_cps.append(face.get_first_char()[0])

    while True:
        all_cps.append(face.get_next_char(all_cps[-1], 0)[0])
        if all_cps[-1] == 0:
            break
    return all_cps


def get_cp_set(args: argparse.Namespace, face: ft.Face):
    """assemble the set of code-points to convert, depending on args"""
    cp_set: set[int] = set()

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


def get_n_ascii(cp_set: list):
    """how many characters correspond to the basic ascii scheme?"""
    first_char = 0
    for i, cp in enumerate(cp_set):
        if i == 0:
            first_char = cp
        elif first_char + i != cp:
            return (first_char, i)
    return (first_char, len(cp_set))


def get_glyph(code: int, face: ft.Face):
    """returns a rendered bitmap of a glyph and its metadata
    the bitmap is in 1 bit / pixel or 8 bit / pixel, depending if a true type font or
    a bitmap font was rendered.
    """
    face.select_charmap(ft.FT_ENCODING_UNICODE)
    face.load_char(code, ft.FT_LOAD_DEFAULT)
    glyph = face.glyph.get_glyph()

    # these values are not valid for outline mode
    metrics = face.glyph.metrics

    blyph = glyph.to_bitmap(ft.FT_RENDER_MODE_NORMAL, ft.Vector(0, 0), True)
    bitmap = blyph.bitmap

    if bitmap.pixel_mode not in (ft.FT_PIXEL_MODE_MONO, ft.FT_PIXEL_MODE_GRAY):
        raise NotImplementedError(f"pixel_mode not supported yet: {bitmap.pixel_mode}")

    props = {
        # number of bytes taken per row
        "pitch": bitmap.pitch,
        # 1 = A monochrome bitmap, using 1 bit per pixel, MSB first,  2 = Each pixel is stored in one byte
        "pixel_mode": bitmap.pixel_mode,
        "width": bitmap.width,
        "height": bitmap.rows,
        "lsb": blyph.left,
        "tsb": blyph.top,
        "advance_hr": face.glyph.advance.x,
        "height_hr": metrics.height,
        "lsb_hr": metrics.horiBearingX,
        "tsb_hr": metrics.horiBearingY,
    }

    return bytes(bitmap.buffer), props


def print_table(vals, w=24, w_v=3, f=None):
    """helper to generate arrays in .h files"""
    ll = len(vals) - 1
    for i, g in enumerate(vals):
        if i > 0 and (i % w) == 0:
            print(file=f)
        print(f"{g:{w_v}d}", end="", file=f)
        if i < ll:
            print(",", end="", file=f)
    print("\n};\n", file=f)


def eight_to_four(buf: bytes):
    """Pack two pixels in one output byte"""
    return bytes(
        (buf[i * 2] & 0xF0) | (buf[i * 2 + 1] >> 4) for i in range(len(buf) // 2)
    )


def four_to_eight(buf: bytes):
    """Unpack two 4-bit pixels into two output bytes"""
    return bytes(
        (buf[i // 2] & 0xF0) if i & 1 else (buf[i // 2] & 0xF) << 4
        for i in range(len(buf) * 2)
    )


def convert(args: argparse.Namespace, face: ft.Face):
    """
    Render all selected glyphs into bitmaps. Return them as bytes and their metadata as a dict.
    """
    cp_set = get_cp_set(args, face)
    if len(cp_set) == 0:
        raise RuntimeError("No glyphs selected to convert")

    # --------------------------------------------------
    #  Generate the glyph bitmap data
    # --------------------------------------------------
    glyph_data_bs = bytes()
    glyph_props = []

    for code in cp_set:
        buf, props = get_glyph(chr(code), face)
        if props["pixel_mode"] == ft.FT_PIXEL_MODE_GRAY and args.four:
            buf = eight_to_four(buf)
        props["cp"] = code
        props["start_index"] = len(glyph_data_bs)
        glyph_props.append(props)
        glyph_data_bs += buf

    # --------------------------------------------------
    #  Generate the codepoint to glyph mapping table
    # --------------------------------------------------
    # convert cp_set to uint32_t
    ascii_map_start, ascii_map_n = get_n_ascii(cp_set)
    map_table = cp_set[ascii_map_n:]
    print(f"    ascii_map_start: 0x{ascii_map_start:02x}, ascii_map_n: {ascii_map_n}")
    print(f"    map_table: {len(map_table)} 32 bit words")
    print("    glyph_data:", len(glyph_data_bs), "bytes")

    # --------------------------------------------------
    #  Collect header info
    # --------------------------------------------------
    # Get the line-height from the numeral bounding box / the OS/2 table
    ls = face.size.height // 64

    if ls == 0:
        print("WARNING!! Setting linespace to 8")
        import pdb

        pdb.set_trace()
        ls = 8

    # Set the flags bits
    flags = FLAGS(0)
    # if args.monospace:
    #     flags |= FLAGS.MONOSPACE
    pix_mode = glyph_props[0]["pixel_mode"]
    if pix_mode == ft.FT_PIXEL_MODE_MONO:
        pass
    elif pix_mode == ft.FT_PIXEL_MODE_GRAY:
        if args.four:
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
        "flags": flags,
        "ascii_map_start": ascii_map_start,
        "ascii_map_n": ascii_map_n,
    }

    return glyph_props, glyph_data_bs, map_table, header


def export_as_fnt(
    glyph_props: list[dict],
    glyph_data_bs: bytes,
    map_table: list[int],
    header: dict,
    out_name: str,
):
    # --------------------------------------------------
    #  Generate the glyph description table
    # --------------------------------------------------
    glyph_description_bs = bytes()
    for i, props in enumerate(glyph_props):
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
        # TODO handle StructError for some glyphs
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
        0,  # header["yshift"] // 64,
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
    glyph_data_bs: bytes,
    map_table: list[int],
    header: dict,
    out_name: str,
):
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

        if header["flags"] == FLAGS(0):
            flag_str = "0"
        else:
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
    .glyph_description_table = glyph_dsc_{name},
    .glyph_data_table = glyphs_{name},
    .flags = {flag_str},
    .name = \"{header['name']}\"
}};
        """,
            file=f,
        )

    print(
        f"wrote {out_name} ({len(glyph_data_bs) / 1024:.1f} kB)",
    )


def glyp_to_img(header: dict, p: dict, glyph_data_bs: bytes, color=0xFFFFFFFF):
    start_ind = p["start_index"]

    # Convert 4 bit mode back to 8 bit mode
    if FLAGS.PIX_FORMAT_B in header["flags"]:
        end_ind = start_ind + (p["width"] * p["height"] // 2 + 1)
        bs = four_to_eight(glyph_data_bs[start_ind:end_ind])
    else:
        end_ind = start_ind + p["width"] * p["height"]
        bs = glyph_data_bs[start_ind:end_ind]

    if p["pixel_mode"] == ft.FT_PIXEL_MODE_MONO:
        # convert 8 pixel / byte buffer to 1 pixel / byte
        bs_ = bytearray(p["width"] * p["height"])
        for y in range(p["height"]):
            for x in range(p["width"]):
                src = bs[y * p["pitch"] + x // 8]
                bit = (src >> (7 - (x % 8))) & 1
                bs_[p["width"] * y + x] = bit * 0xFF
    elif p["pixel_mode"] == ft.FT_PIXEL_MODE_GRAY:
        bs_ = bs
    else:
        NotImplementedError("pixel mode not supported")

    img = Image.frombuffer("L", (p["width"], p["height"]), bs_)
    img_ = Image.new("RGBA", img.size, color)
    img_.putalpha(img)
    return img_


def export_as_png(
    glyph_props: list[dict],
    glyph_data_bs: bytes,
    header: dict,
    out_name: str,
):
    all_inds = range(len(glyph_props))

    print(glyph_props[0])

    # Measure the width needed to print all the glyphs
    total_advance = 0
    for ind in all_inds:
        total_advance += glyph_props[ind]["advance_hr"] // 64

    img_all = Image.new(
        "RGBA", (total_advance + 8, int(header["linespace"] * 1.5)), 0xFF000000
    )

    # Draw a red line where the font baseline is
    draw = ImageDraw.Draw(img_all)
    draw.line(
        (0, header["linespace"], img_all.size[0], header["linespace"]), fill=0xFF0000FF
    )

    cur_x = 4
    for ind in all_inds:  # for each glyph
        p = glyph_props[ind]
        # Draw the bitmap bounding box in blue
        draw.rectangle(
            (
                cur_x + p["lsb"],
                -p["tsb"] + header["linespace"],
                cur_x + p["lsb"] + p["width"],
                -p["tsb"] + header["linespace"] + p["height"],
            ),
            outline=0xFF880000,
        )
        # Place the glyph in the same location as font.c would
        img_all.alpha_composite(
            glyp_to_img(header, p, glyph_data_bs, 0xFFFFFFFF),
            (
                cur_x + p["lsb"],
                -p["tsb"] + header["linespace"],
            ),
        )
        cur_x += p["advance_hr"] // 64

    img_all.save(out_name, "PNG")
    print("wrote", out_name)


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument(
        "--height",
        type=float,
        help="Target height of the glyphs. Default for vector fonts: 30 pixels",
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
        "--four",
        action="store_true",
        help="Convert 8-bit greyscale to 4-bit greyscale. Ignored for 1 bit bitmap font inputs.",
    )
    # parser.add_argument(
    #     "--monospace",
    #     action="store_true",
    #     help="In monospace mode, all glyphs are forced to the same properties",
    # )

    args = parser.parse_args()

    # Load an existing font file and set its size to user input / a reasonable default
    face = ft.Face(args.fontfile)
    if args.height is None:
        if face.is_scalable:
            args.height = 30
        else:
            args.height = face.available_sizes[-1].height
    try:
        face.set_char_size(height=int(args.height * 64))
    except ft.FT_Exception:
        print(
            "ERROR: Selected height not one of the supported ones:",
            ",".join([str(ho.height) for ho in face.available_sizes]),
        )
        exit(-1)

    # generate bitmap font
    glyph_props, glyph_data, map_table, header = convert(args, face)

    # Generate the output file name
    out_name = Path("fnt/")
    out_name.mkdir(exist_ok=True)
    f_ending = ".fnt" if args.fnt else ".h"
    if args.numeric_name:
        out_name = get_next_filename(out_name, f_ending)
    else:
        tmp = re.sub("[^A-Za-z0-9]+", "_", face.family_name.decode().lower())
        out_name = (out_name / tmp).with_suffix(f_ending)

    if args.fnt:
        print(f"\nGenerating binary .fnt file ...")
        export_as_fnt(glyph_props, glyph_data, map_table, header, str(out_name))
    else:
        print(f"\nGenerating C-header .h file ...")
        export_as_header(glyph_props, glyph_data, map_table, header, str(out_name))

    export_as_png(glyph_props, glyph_data, header, str(out_name.with_suffix(".png")))


if __name__ == "__main__":
    main()
