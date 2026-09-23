#!/usr/bin/env python3
"""Headless display-pipeline check: baked segmap + live Python VRAM -> PPM.

1. Parses portmaster-build/include/brickemu/Apollo18Segments.hpp.
2. Renders an all-segments-on frame (geometry sanity: fill ratio, bounds).
3. Boots the real Python SPL03 core, runs 30k clocks (attract mode), renders
   the live LCDRAM frame.
4. Writes /tmp/opencode/lcd_all.ppm and /tmp/opencode/lcd_live.ppm.

No graphics dependencies; verifies baker output + proves VRAM carries a
real picture before any SDL2 work.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

HDR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                   "portmaster-build", "include", "brickemu",
                   "Apollo18Segments.hpp")

W, H = None, None
SEGS = []
for m in re.finditer(r"inline constexpr std::uint16_t kApollo18Canvas([WH]) = (\d+);",
                     open(HDR).read()):
    if m.group(1) == "W":
        W = int(m.group(2))
    else:
        H = int(m.group(2))
for m in re.finditer(r"\{(\d+), (\d+), (\d+), (\d+), (\d+), (\d+)\}",
                     open(HDR).read()):
    SEGS.append(tuple(int(g) for g in m.groups()))
assert (W, H) == (332, 480), (W, H)
assert len(SEGS) == 237, len(SEGS)

BG = (156, 166, 108)  # classic LCD green-gray
FG = (30, 34, 26)


def render(lcdram):
    px = bytearray(BG * 1)
    buf = bytearray(W * H * 3)
    for i in range(W * H):
        buf[3 * i:3 * i + 3] = bytes(BG)
    lit = 0
    for x, y, w, h, byte, bit in SEGS:
        assert 0 <= x < W and 0 <= y < H and x + w <= W and y + h <= H, (x, y, w, h)
        assert w > 0 and h > 0
        if byte < len(lcdram) and (lcdram[byte] >> bit) & 1:
            lit += 1
            for yy in range(y, y + h):
                base = (yy * W + x) * 3
                for xx in range(w):
                    buf[base + 3 * xx:base + 3 * xx + 3] = bytes(FG)
    return bytes(buf), lit


def write_ppm(path, buf):
    with open(path, "wb") as f:
        f.write(f"P6\n{W} {H}\n255\n".encode() + buf)


os.makedirs("/tmp/opencode", exist_ok=True)
all_on, lit = render([0xFF] * 0x30)
write_ppm("/tmp/opencode/lcd_all.ppm", all_on)
fg_px = sum(1 for i in range(0, len(all_on), 3)
            if tuple(all_on[i:i + 3]) == FG)
print(f"all-on: {lit}/237 lit, fg fill {fg_px / (W * H) * 100:.1f}%")

from cores.SPL03 import SPL03
from interconnect import Interconnect


class FakeEmu:
    def audio_handler(self, c, d):
        pass

    def serial_tx_handler(self, d):
        pass


cpu = SPL03({"rom_path": "assets/Apollo18in1B0302.bin",
             "port_pullup": {"PA": 0}, "port_pkey": {"PA": 1},
             "non_crystal_div": 16},
            690000, Interconnect(FakeEmu()))
for _ in range(200000):
    cpu.clock()
live, lit = render(cpu.get_VRAM())
write_ppm("/tmp/opencode/lcd_live.ppm", live)
print(f"live@200k: {lit}/237 segments lit")
print("wrote /tmp/opencode/lcd_all.ppm /tmp/opencode/lcd_live.ppm")
