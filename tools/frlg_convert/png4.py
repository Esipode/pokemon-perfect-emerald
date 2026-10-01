"""Minimal indexed-PNG codec (standard library only) for 8x8 tile sheets.

Reads palette-indexed PNGs at bit depth 4 or 8 (non-interlaced) and writes
4-bit indexed PNGs. Tiles are flat 64-byte sequences of colour indices,
ordered row-major across the sheet.
"""

import struct
import zlib

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
TILE_SIZE = 8
TILE_PIXELS = TILE_SIZE * TILE_SIZE


class PngError(Exception):
    pass


def _chunks(data):
    if data[:8] != PNG_SIGNATURE:
        raise PngError("not a PNG file")
    pos = 8
    while pos < len(data):
        (length,) = struct.unpack(">I", data[pos:pos + 4])
        ctype = data[pos + 4:pos + 8]
        yield ctype, data[pos + 8:pos + 8 + length]
        pos += 12 + length


def _paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def _unfilter(raw, height, stride, bpp):
    out = bytearray()
    prev = bytearray(stride)
    pos = 0
    for _ in range(height):
        ftype = raw[pos]
        line = bytearray(raw[pos + 1:pos + 1 + stride])
        pos += 1 + stride
        if ftype == 1:
            for i in range(bpp, stride):
                line[i] = (line[i] + line[i - bpp]) & 0xFF
        elif ftype == 2:
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 0xFF
        elif ftype == 3:
            for i in range(stride):
                left = line[i - bpp] if i >= bpp else 0
                line[i] = (line[i] + ((left + prev[i]) >> 1)) & 0xFF
        elif ftype == 4:
            for i in range(stride):
                left = line[i - bpp] if i >= bpp else 0
                upleft = prev[i - bpp] if i >= bpp else 0
                line[i] = (line[i] + _paeth(left, prev[i], upleft)) & 0xFF
        elif ftype != 0:
            raise PngError(f"bad filter type {ftype}")
        out += line
        prev = line
    return out


class IndexedImage:
    """Indexed image: width, height, pixels (bytes of indices, row-major), PLTE bytes."""

    def __init__(self, width, height, pixels, plte):
        self.width = width
        self.height = height
        self.pixels = pixels
        self.plte = plte


def read_png(data):
    """Decode PNG bytes into an IndexedImage."""
    ihdr = None
    plte = b""
    idat = bytearray()
    for ctype, body in _chunks(data):
        if ctype == b"IHDR":
            ihdr = struct.unpack(">IIBBBBB", body)
        elif ctype == b"PLTE":
            plte = body
        elif ctype == b"IDAT":
            idat += body
        elif ctype == b"IEND":
            break
    if ihdr is None:
        raise PngError("missing IHDR")
    width, height, depth, ctype, _, _, interlace = ihdr
    if ctype != 3:
        raise PngError(f"colour type {ctype} is not indexed")
    if depth not in (4, 8):
        raise PngError(f"unsupported bit depth {depth}")
    if interlace:
        raise PngError("interlaced PNG not supported")
    stride = (width * depth + 7) // 8
    raw = _unfilter(zlib.decompress(bytes(idat)), height, stride, 1)
    if depth == 8:
        pixels = bytes(raw)
    else:
        pixels = bytearray(width * height)
        for y in range(height):
            row = raw[y * stride:(y + 1) * stride]
            base = y * width
            for x in range(width):
                byte = row[x >> 1]
                pixels[base + x] = (byte >> 4) if (x & 1) == 0 else (byte & 0xF)
        pixels = bytes(pixels)
    return IndexedImage(width, height, pixels, plte)


def image_to_tiles(img):
    """Split an image into 8x8 tiles, row-major."""
    if img.width % TILE_SIZE or img.height % TILE_SIZE:
        raise PngError(f"image size {img.width}x{img.height} is not a multiple of 8")
    cols = img.width // TILE_SIZE
    rows = img.height // TILE_SIZE
    tiles = []
    for ty in range(rows):
        for tx in range(cols):
            tile = bytearray(TILE_PIXELS)
            for y in range(TILE_SIZE):
                start = (ty * TILE_SIZE + y) * img.width + tx * TILE_SIZE
                tile[y * TILE_SIZE:(y + 1) * TILE_SIZE] = img.pixels[start:start + TILE_SIZE]
            tiles.append(bytes(tile))
    return tiles


def read_tiles(data):
    """Decode PNG bytes into (tiles, PLTE bytes)."""
    img = read_png(data)
    return image_to_tiles(img), img.plte


def _chunk(ctype, body):
    return struct.pack(">I", len(body)) + ctype + body + struct.pack(">I", zlib.crc32(ctype + body) & 0xFFFFFFFF)


def write_tiles_png(tiles, plte, tiles_per_row=16):
    """Encode tiles as a 4-bit indexed PNG. Pads the last row with blank tiles."""
    tiles = list(tiles)
    if len(tiles) % tiles_per_row:
        tiles += [bytes(TILE_PIXELS)] * (tiles_per_row - len(tiles) % tiles_per_row)
    rows = len(tiles) // tiles_per_row
    width = tiles_per_row * TILE_SIZE
    height = rows * TILE_SIZE
    raw = bytearray()
    for ty in range(rows):
        for y in range(TILE_SIZE):
            raw.append(0)
            for tx in range(tiles_per_row):
                line = tiles[ty * tiles_per_row + tx][y * TILE_SIZE:(y + 1) * TILE_SIZE]
                for x in range(0, TILE_SIZE, 2):
                    if line[x] > 15 or line[x + 1] > 15:
                        raise PngError("colour index above 15 in 4-bit output")
                    raw.append((line[x] << 4) | line[x + 1])
    if not plte:
        plte = bytes(16 * 3)
    ihdr = struct.pack(">IIBBBBB", width, height, 4, 3, 0, 0, 0)
    return (PNG_SIGNATURE + _chunk(b"IHDR", ihdr) + _chunk(b"PLTE", plte)
            + _chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + _chunk(b"IEND", b""))
