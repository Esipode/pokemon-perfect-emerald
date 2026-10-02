#!/usr/bin/env python3
"""Trim leading/trailing near-silence from non-Mega cry WAVs.

Usage: trim_cries.py [--apply] [--threshold N]
Without --apply, only reports the savings. Files with loop/sampler chunks and
16-bit files are skipped. A 5 ms fade-out is applied to the new tail.
"""
import argparse
import glob
import os
import struct

CRY_DIR = os.path.join(os.path.dirname(__file__), "..", "..", "sound", "direct_sound_samples", "cries")


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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("--threshold", type=int, default=2)
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
        if channels != 1 or bits != 8:
            continue
        pcm = bytearray(chunks[b"data"])
        loud = [i for i, s in enumerate(pcm) if abs(s - 128) > args.threshold]
        if not loud:
            continue
        start, end = loud[0], loud[-1] + 1
        if start == 0 and end == len(pcm):
            continue
        out = bytearray(pcm[start:end])
        fade = min(len(out), max(1, rate * 5 // 1000))
        for k in range(fade):
            j = len(out) - fade + k
            out[j] = 128 + round((out[j] - 128) * (fade - 1 - k) / fade)
        saved = len(pcm) - len(out)
        total += saved
        count += 1
        if args.apply:
            open(path, "wb").write(build(fmt, bytes(out)))
    print(f"{count} files, {total} sample bytes trimmed (raw; ROM saving is ~half after DPCM)")


if __name__ == "__main__":
    main()
