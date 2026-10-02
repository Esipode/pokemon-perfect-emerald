#!/usr/bin/env python3
"""Resample 13,379 Hz and 13,000 Hz non-Mega cry WAVs to 10,512 Hz.

Usage: resample_cries.py [--apply] [--target RATE]
Without --apply, only reports the savings. Windowed-sinc low-pass resampling.
Files with extra chunks and 16-bit files are skipped.
"""
import argparse
import glob
import math
import os
import struct

CRY_DIR = os.path.join(os.path.dirname(__file__), "..", "..", "sound", "direct_sound_samples", "cries")
HALF = 12


def parse(data):
    chunks = {}
    order = []
    i = 12
    while i + 8 <= len(data):
        name = data[i:i + 4]
        size = struct.unpack("<I", data[i + 4:i + 8])[0]
        chunks[name] = data[i + 8:i + 8 + size]
        order.append(name)
        i += 8 + size + (size & 1)
    return chunks, order


def build(fmt, pcm):
    body = b"fmt " + struct.pack("<I", len(fmt)) + fmt
    if len(fmt) & 1:
        body += b"\0"
    body += b"data" + struct.pack("<I", len(pcm)) + pcm
    if len(pcm) & 1:
        body += b"\0"
    return b"RIFF" + struct.pack("<I", len(body) + 4) + b"WAVE" + body


def resample(pcm, src, dst):
    ratio = dst / src
    fc = 0.45 * ratio
    n_out = max(1, round(len(pcm) * ratio))
    x = [s - 128 for s in pcm]
    n = len(x)
    out = bytearray(n_out)
    for o in range(n_out):
        t = o / ratio
        lo = max(0, math.ceil(t - HALF))
        hi = min(n - 1, math.floor(t + HALF))
        acc = 0.0
        norm = 0.0
        for k in range(lo, hi + 1):
            d = k - t
            a = 2 * fc * d
            sinc = 1.0 if a == 0 else math.sin(math.pi * a) / (math.pi * a)
            w = 0.5 + 0.5 * math.cos(math.pi * d / (HALF + 1))
            h = sinc * w
            acc += x[k] * h
            norm += h
        v = round(acc / norm) if norm else 0
        out[o] = 128 + max(-128, min(127, v))
    return bytes(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("--target", type=int, default=10512)
    args = ap.parse_args()

    total = 0
    count = 0
    for path in sorted(glob.glob(os.path.join(CRY_DIR, "*.wav"))):
        if path.endswith("_mega.wav") or "_mega_" in os.path.basename(path):
            continue
        data = open(path, "rb").read()
        chunks, order = parse(data)
        if order != [b"fmt ", b"data"]:
            continue
        fmt = chunks[b"fmt "]
        _, channels, rate, _, _, bits = struct.unpack("<HHIIHH", fmt[:16])
        if channels != 1 or bits != 8 or rate not in (13379, 13000):
            continue
        pcm = chunks[b"data"]
        out = resample(pcm, rate, args.target)
        newfmt = bytearray(fmt)
        struct.pack_into("<II", newfmt, 4, args.target, args.target)
        total += len(pcm) - len(out)
        count += 1
        if args.apply:
            open(path, "wb").write(build(bytes(newfmt), out))
    print(f"{count} files, {total} sample bytes removed (raw; ROM saving is ~half after DPCM)")


if __name__ == "__main__":
    main()
