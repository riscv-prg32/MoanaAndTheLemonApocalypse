#!/usr/bin/env python3
"""Generate original PNG/WAV source assets with only the Python standard library."""

from __future__ import annotations

import json
import math
import struct
import wave
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PNG = ROOT / "png"
WAV = ROOT / "wav"
PNG.mkdir(parents=True, exist_ok=True)
WAV.mkdir(parents=True, exist_ok=True)

SKY = (126, 199, 255, 255)
SEA = (0, 152, 190, 255)
FOAM = (128, 225, 255, 255)
GRASS = (80, 190, 92, 255)
GRASS_DK = (35, 132, 52, 255)
SAND = (252, 215, 122, 255)
TREE = (37, 144, 68, 255)
TREE_DK = (26, 104, 48, 255)
TRUNK = (136, 82, 36, 255)
LEMON = (255, 235, 62, 255)
MAGIC = (68, 232, 255, 255)
ENERGY = (255, 160, 42, 255)
BAD = (104, 84, 118, 255)
SKIN = (248, 193, 135, 255)
HAIR = (54, 34, 28, 255)
DRESS = (24, 174, 190, 255)
CHICKEN = (255, 246, 185, 255)
ROOSTER = (222, 36, 28, 255)
BLUE = (40, 78, 190, 255)
BLACK = (0, 0, 0, 255)
WHITE = (255, 255, 255, 255)
CLEAR = (0, 0, 0, 0)


class Canvas:
    def __init__(self, w: int, h: int, color=CLEAR):
        self.w = w
        self.h = h
        self.p = [color] * (w * h)

    def px(self, x: int, y: int, c) -> None:
        if 0 <= x < self.w and 0 <= y < self.h:
            self.p[y * self.w + x] = c

    def rect(self, x0: int, y0: int, x1: int, y1: int, c) -> None:
        for y in range(max(0, y0), min(self.h, y1 + 1)):
            for x in range(max(0, x0), min(self.w, x1 + 1)):
                self.px(x, y, c)

    def ellipse(self, x0: int, y0: int, x1: int, y1: int, c) -> None:
        rx = max(1, (x1 - x0) / 2)
        ry = max(1, (y1 - y0) / 2)
        cx = x0 + rx
        cy = y0 + ry
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1:
                    self.px(x, y, c)

    def paste(self, other: "Canvas", ox: int, oy: int) -> None:
        for y in range(other.h):
            for x in range(other.w):
                c = other.p[y * other.w + x]
                if c[3]:
                    self.px(ox + x, oy + y, c)

    def scale(self, w: int, h: int) -> "Canvas":
        out = Canvas(w, h)
        for y in range(h):
            for x in range(w):
                out.p[y * w + x] = self.p[(y * self.h // h) * self.w + (x * self.w // w)]
        return out

    def save_png(self, path: Path) -> None:
        raw = bytearray()
        for y in range(self.h):
            raw.append(0)
            for r, g, b, a in self.p[y * self.w:(y + 1) * self.w]:
                raw.extend((r, g, b, a))
        def chunk(kind: bytes, data: bytes) -> bytes:
            return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xffffffff)
        data = b"\x89PNG\r\n\x1a\n"
        data += chunk(b"IHDR", struct.pack(">IIBBBBB", self.w, self.h, 8, 6, 0, 0, 0))
        data += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
        data += chunk(b"IEND", b"")
        path.write_bytes(data)


def draw_tree(c: Canvas, x: int, y: int) -> None:
    c.rect(x - 4, y + 25, x + 4, y + 62, TRUNK)
    c.ellipse(x - 26, y + 10, x + 8, y + 38, TREE)
    c.ellipse(x - 7, y, x + 27, y + 39, TREE_DK)
    c.ellipse(x - 20, y + 22, x + 24, y + 50, TREE)
    for lx, ly in [(-13, 20), (2, 14), (14, 28), (-2, 34), (-20, 34)]:
        c.ellipse(x + lx, y + ly, x + lx + 6, y + ly + 6, LEMON)


def background() -> Canvas:
    c = Canvas(320, 200, SKY)
    c.rect(0, 0, 319, 17, SEA)
    c.rect(0, 18, 319, 29, FOAM)
    c.rect(0, 140, 319, 189, GRASS)
    c.rect(0, 188, 319, 199, SAND)
    for x in range(0, 320, 16):
        c.rect(x, 176 + (x // 16) % 4, x + 12, 178 + (x // 16) % 4, GRASS_DK)
    for x, y in [(36, 42), (98, 28), (160, 48), (226, 32), (284, 50)]:
        draw_tree(c, x, y)
    return c


def splash() -> Canvas:
    c = background()
    c.rect(22, 32, 298, 150, LEMON)
    c.rect(26, 36, 294, 146, (3, 41, 56, 255))
    for x in range(48, 272, 8):
        c.rect(x, 58 + (x // 8) % 2, x + 4, 64 + (x // 8) % 2, WHITE)
        c.rect(x, 78 + (x // 8) % 2, x + 5, 84 + (x // 8) % 2, LEMON)
    c.rect(64, 104, 256, 108, WHITE)
    c.rect(82, 122, 238, 126, MAGIC)
    c.ellipse(132, 154, 188, 190, LEMON)
    c.rect(160, 151, 180, 159, TREE)
    return c


def moana_frame(frame: int) -> Canvas:
    c = Canvas(16, 16)
    c.rect(3, 0, 12, 5, HAIR)
    c.rect(4, 2, 12, 8, SKIN)
    c.px(6, 4, BLACK); c.px(11, 4, BLACK)
    c.rect(2, 8, 14, 13, DRESS)
    c.rect(1, 10, 4, 13, SKIN); c.rect(12, 10, 15, 13, SKIN)
    c.rect(12, 6, 15, 10, TRUNK); c.ellipse(13, 7, 15, 9, LEMON)
    c.rect(4, 13, 6, 15 if frame & 1 else 14, BLACK)
    c.rect(9, 13, 11, 14 if frame & 1 else 15, BLACK)
    return c


def chicken_frame(frame: int) -> Canvas:
    c = Canvas(16, 16)
    c.ellipse(4, 4, 13, 12, CHICKEN)
    c.ellipse(10, 2, 15, 7, CHICKEN)
    c.rect(14, 4, 15, 6, ENERGY)
    c.px(12, 4, BLACK)
    c.ellipse(1 if frame & 1 else 2, 7, 7, 10, WHITE)
    c.rect(5, 12, 6, 15, ENERGY); c.rect(10, 12, 11, 15, ENERGY)
    return c


def rooster_frame(frame: int) -> Canvas:
    c = Canvas(16, 16)
    c.ellipse(3, 5, 13, 13, ROOSTER)
    c.ellipse(9, 1, 15, 7, ROOSTER)
    c.rect(10, 0, 14, 2, ENERGY)
    c.rect(14, 4, 15, 6, LEMON)
    c.px(12, 3, BLACK)
    c.ellipse(-3, 5, 8, 15, BLUE)
    c.rect(5, 13, 6, 15, ENERGY); c.rect(11, 13, 12, 15, ENERGY)
    return c


def lemon_frame(kind: str) -> Canvas:
    colors = {"normal": LEMON, "magic": MAGIC, "energy": ENERGY, "bad": BAD}
    c = Canvas(16, 16)
    c.ellipse(3, 4, 12, 12, colors[kind])
    c.ellipse(6, 3, 14, 11, colors[kind])
    c.rect(10, 2, 14, 4, TREE)
    if kind == "bad":
        c.rect(6, 8, 11, 10, BLACK)
    return c


def sheet(frames: list[Canvas], name: str) -> None:
    out = Canvas(16 * len(frames), 16)
    for i, frame in enumerate(frames):
        out.paste(frame, i * 16, 0)
    out.save_png(PNG / name)


def wav(name: str, notes: list[tuple[float, float]], rate: int = 22050) -> None:
    data = bytearray()
    for freq, dur in notes:
        n_samples = int(rate * dur)
        for n in range(n_samples):
            if freq == 0:
                value = 0
            else:
                t = n / rate
                env = min(1.0, n / (rate * 0.01), (n_samples - n) / (rate * 0.02))
                value = int(math.sin(2 * math.pi * freq * t) * env * 18000)
            data.extend(struct.pack("<h", value))
    with wave.open(str(WAV / name), "wb") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(rate)
        f.writeframes(data)


def main() -> None:
    bg = background()
    bg.save_png(PNG / "background_lemon_grove_320x200.png")
    sp = splash()
    sp.save_png(PNG / "splash_moana_lemon_apocalypse_320x200.png")
    sp.scale(128, 80).save_png(ROOT / "screenshot.png")

    icon = Canvas(64, 64)
    icon.ellipse(8, 14, 54, 52, LEMON)
    icon.rect(36, 8, 54, 16, TREE)
    icon.rect(25, 28, 29, 42, (3, 41, 56, 255))
    icon.rect(34, 28, 38, 42, (3, 41, 56, 255))
    icon.rect(29, 32, 34, 36, (3, 41, 56, 255))
    icon.save_png(ROOT / "icon.png")

    sheet([moana_frame(i) for i in range(4)], "sprite_moana_4frames_16x16.png")
    sheet([chicken_frame(i) for i in range(4)], "sprite_chicken_4frames_16x16.png")
    sheet([rooster_frame(i) for i in range(4)], "sprite_rooster_4frames_16x16.png")
    sheet([lemon_frame(k) for k in ["normal", "magic", "energy", "bad"]], "sprite_lemons_4kinds_16x16.png")

    melody = [(392, .18), (392, .18), (440, .18), (392, .18), (330, .30), (0, .10),
              (330, .18), (349, .18), (392, .18), (349, .18), (330, .30)]
    wav("soundtrack_lemon_tree_inspired_loop.wav", melody * 4)
    wav("sfx_collect_lemon.wav", [(660, .08), (880, .08)])
    wav("sfx_rooster_scared.wav", [(1047, .12), (784, .08)])
    wav("sfx_bad_lemon.wav", [(110, .18)])
    wav("sfx_game_over.wav", [(165, .2), (110, .35)])

    manifest = {
        "title": "Moana and the Lemon Apocalypse asset manifest",
        "license": "MIT / original generated assets",
        "png": sorted(p.name for p in PNG.glob("*.png")),
        "wav": sorted(p.name for p in WAV.glob("*.wav")),
        "sprite_format": {
            "sprite_*_4frames_16x16.png": "4 horizontal frames, each 16x16 RGBA",
            "sprite_lemons_4kinds_16x16.png": "normal, magic, energy, bad lemon frames"
        },
        "music_note": "Original chiptune motif inspired by the cheerful feel of Lemon Tree; no copied recording or song file is bundled."
    }
    (ROOT / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    main()
