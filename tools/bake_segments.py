#!/usr/bin/env python3
"""Bake Apollo18 SVG segment geometry into a compact C++ segment map.

Reads assets/Apollo18in1B0302.svg, computes tight bounding boxes for every
`{byte}_{bit}` segment element (231 <path> + 6 <g> of paths; commands used:
M, C, L, H, V, Z with cubic-bezier extrema solved exactly), crops to the
segments extent, normalizes into an integer LCD canvas (height 480, aspect
preserved) and emits a header with one rect per segment:

    struct Seg { uint16_t x, y, w, h; uint8_t byte, bit; };

The emulator runtime never touches SVG: ~2.4 KB static table, zero parsing
on handhelds (RG35XX H class, 1 GB RAM).

Usage:
    python3 tools/bake_segments.py
"""
import math
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SVG_PATH = os.path.join(ROOT, "assets", "Apollo18in1B0302.svg")
OUT_PATH = os.path.join(ROOT, "portmaster-build", "include", "brickemu",
                        "Apollo18Segments.hpp")

CANVAS_H = 480


def cubic_extrema(p0, p1, p2, p3):
    """t values in [0,1] where a cubic bezier component has zero derivative."""
    a = -p0 + 3 * p1 - 3 * p2 + p3
    b = 2 * (p0 - 2 * p1 + p2)
    c = -p0 + p1
    if abs(a) < 1e-12:
        if abs(b) < 1e-12:
            return []
        t = -c / b
        return [t] if 0.0 < t < 1.0 else []
    disc = b * b - 4 * a * c
    if disc < 0:
        return []
    sq = math.sqrt(disc)
    out = []
    for t in ((-b + sq) / (2 * a), (-b - sq) / (2 * a)):
        if 0.0 < t < 1.0:
            out.append(t)
    return out


def cubic_at(p0, p1, p2, p3, t):
    u = 1 - t
    return u*u*u*p0 + 3*u*u*t*p1 + 3*u*t*t*p2 + t*t*t*p3


def path_bbox(d):
    """Tight bbox of an SVG path using only M,C,L,H,V,Z (absolute+relative)."""
    toks = re.findall(r"[MCLHVZmclhvz]|-?\d*\.?\d+(?:[eE][-+]?\d+)?", d.replace(",", " "))
    xs = []
    ys = []
    i = 0
    cx = cy = 0.0
    sx = sy = 0.0  # subpath start

    def pt():
        nonlocal i
        x = float(toks[i])
        y = float(toks[i + 1])
        i += 2
        return x, y

    cmd = None
    while i < len(toks):
        if toks[i].isalpha():
            cmd = toks[i]
            i += 1
        if cmd in ("M", "m"):
            x, y = pt()
            if cmd == "m":
                x += cx
                y += cy
            cx, cy = sx, sy = x, y
            xs.append(x)
            ys.append(y)
            cmd = "l" if cmd == "m" else "L"  # subsequent pairs are lineto
        elif cmd in ("L", "l"):
            x, y = pt()
            if cmd == "l":
                x += cx
                y += cy
            cx, cy = x, y
            xs.append(x)
            ys.append(y)
        elif cmd in ("H", "h"):
            x = float(toks[i])
            i += 1
            cx = x if cmd == "H" else cx + x
            xs.append(cx)
            ys.append(cy)
        elif cmd in ("V", "v"):
            y = float(toks[i])
            i += 1
            cy = y if cmd == "V" else cy + y
            xs.append(cx)
            ys.append(cy)
        elif cmd in ("C", "c"):
            pts = [pt(), pt(), pt()]
            if cmd == "c":
                pts = [(cx + x, cy + y) for x, y in pts]
            (x1, y1), (x2, y2), (x, y) = pts
            for t in cubic_extrema(cx, x1, x2, x) + [0.0, 1.0]:
                xs.append(cubic_at(cx, x1, x2, x, t))
            for t in cubic_extrema(cy, y1, y2, y) + [0.0, 1.0]:
                ys.append(cubic_at(cy, y1, y2, y, t))
            cx, cy = x, y
        elif cmd in ("Z", "z"):
            cx, cy = sx, sy
        else:
            raise ValueError(f"unsupported path command: {cmd}")
    return (min(xs), min(ys), max(xs), max(ys))


def main():
    svg = open(SVG_PATH).read()

    # path segments: id="B_b"
    segs = {}  # (byte, bit) -> bbox
    for m in re.finditer(r'<path[^>]*>', svg):
        tag = m.group(0)
        im = re.search(r'id="(\d+)_(\d+)"', tag)
        dm = re.search(r'[\s]d="([^"]+)"', tag)
        if im and dm:
            segs[(int(im.group(1)), int(im.group(2)))] = path_bbox(dm.group(1))

    # group segments: <g id="B_b"> ... child paths ... </g>
    for m in re.finditer(r'<g[^>]*id="(\d+)_(\d+)"[^>]*>(.*?)</g>', svg, re.S):
        byte, bit, inner = int(m.group(1)), int(m.group(2)), m.group(3)
        boxes = [path_bbox(dm.group(1))
                 for dm in re.finditer(r'<path[^>]*d="([^"]+)"', inner)]
        if not boxes:
            print(f"WARNING: group {byte}_{bit} has no path children",
                  file=sys.stderr)
            continue
        segs[(byte, bit)] = (
            min(b[0] for b in boxes), min(b[1] for b in boxes),
            max(b[2] for b in boxes), max(b[3] for b in boxes))

    print(f"segments: {len(segs)}")
    assert len(segs) == 237, f"expected 237 segments, got {len(segs)}"

    x0 = min(b[0] for b in segs.values())
    y0 = min(b[1] for b in segs.values())
    x1 = max(b[2] for b in segs.values())
    y1 = max(b[3] for b in segs.values())
    print(f"extent: x=[{x0:.1f},{x1:.1f}] y=[{y0:.1f},{y1:.1f}] "
          f"w={x1-x0:.1f} h={y1-y0:.1f}")

    scale = CANVAS_H / (y1 - y0)
    canvas_w = max(1, round((x1 - x0) * scale))
    print(f"canvas: {canvas_w}x{CANVAS_H} (scale {scale:.4f})")

    entries = []
    for (byte, bit) in sorted(segs):
        bx0, by0, bx1, by1 = segs[(byte, bit)]
        x = int((bx0 - x0) * scale)
        y = int((by0 - y0) * scale)
        w = max(1, int(round((bx1 - bx0) * scale)))
        h = max(1, int(round((by1 - by0) * scale)))
        x = min(x, canvas_w - 1)
        y = min(y, CANVAS_H - 1)
        w = min(w, canvas_w - x)
        h = min(h, CANVAS_H - y)
        entries.append((x, y, w, h, byte, bit))

    used_bytes = sorted({b for _, _, _, _, b, _ in entries})
    print(f"LCDRAM bytes used: {len(used_bytes)}")

    # Playfield: the 10x17 regular block matrix (photo-verified). Detect by
    # cell size, then snap centers into 10 column / 17 row bands.
    svg_boxes = {}
    for m in re.finditer(r'<path[^>]*>', svg):
        tag = m.group(0)
        im = re.search(r'id="(\d+)_(\d+)"', tag)
        dm = re.search(r'[\s]d="([^"]+)"', tag)
        if im and dm:
            svg_boxes[(int(im.group(1)), int(im.group(2)))] = path_bbox(dm.group(1))
    for m in re.finditer(r'<g[^>]*id="(\d+)_(\d+)"[^>]*>(.*?)</g>', svg, re.S):
        bb = [path_bbox(dm.group(1))
              for dm in re.finditer(r'<path[^>]*d="([^"]+)"', m.group(3))]
        svg_boxes[(int(m.group(1)), int(m.group(2)))] = (
            min(b[0] for b in bb), min(b[1] for b in bb),
            max(b[2] for b in bb), max(b[3] for b in bb))
    pf = {k: b for k, b in svg_boxes.items()
          if 58 <= b[2] - b[0] <= 68 and 64 <= b[3] - b[1] <= 74}
    assert len(pf) == 170, f"expected 170 playfield cells, got {len(pf)}"

    def bands(vals, tol=12.0):
        groups = []
        for x in sorted(vals):
            if groups and x - groups[-1][-1] <= tol:
                groups[-1].append(x)
            else:
                groups.append([x])
        return [sum(g) / len(g) for g in groups]

    cx = {(b[0] + b[2]) / 2 for b in pf.values()}
    cy = {(b[1] + b[3]) / 2 for b in pf.values()}
    # NOTE: sets collapse duplicates; grid regularity is verified below.
    xs = bands(list(cx))
    ys = bands(list(cy))
    assert len(xs) == 10, f"x bands: {len(xs)}"
    assert len(ys) == 17, f"y bands: {len(ys)}"
    xs.sort()
    ys.sort()

    def nearest_idx(v, edges):
        return min(range(len(edges)), key=lambda i: abs(v - edges[i]))

    pfmap = {}
    for (byte, bit), (x0, y0, x1, y1) in pf.items():
        col = nearest_idx((x0 + x1) / 2, xs)
        row = nearest_idx((y0 + y1) / 2, ys)
        assert (row, col) not in pfmap, f"collision at {(row, col)}"
        pfmap[(row, col)] = (byte, bit)
    assert len(pfmap) == 170, "playfield grid incomplete"
    assert len(set(pfmap.values())) == 170, "playfield bits not unique"
    assert all(b < 0x30 for b, _ in pfmap.values())
    print(f"playfield: 10x17 grid, x0={xs[0]:.0f} y0={ys[0]:.0f} "
          f"pitch={xs[1] - xs[0]:.1f}x{ys[1] - ys[0]:.1f}")

    # HUD map: score digits, speed/level digits, icons. Cluster the 67
    # non-playfield segments by x-overlap; 7-segment digits get role order
    # (top, upper-left, upper-right, mid, lower-left, lower-right, bottom)
    # from geometry (wide segments split top/mid/bottom by height, tall ones
    # split left/right then upper/lower).
    rest = {k: v for k, v in svg_boxes.items() if k not in pf}
    parent = {k: k for k in rest}

    def find(a):
        while parent[a] != a:
            parent[a] = parent[parent[a]]
            a = parent[a]
        return a

    rkeys = list(rest)
    for i, a in enumerate(rkeys):
        for b in rkeys[i + 1:]:
            ax0, _, ax1, _ = rest[a]
            bx0, _, bx1, _ = rest[b]
            if not (ax1 + 8 < bx0 or bx1 + 8 < ax0):
                parent[find(a)] = find(b)
    from collections import defaultdict
    clusters = defaultdict(list)
    for k in rkeys:
        clusters[find(k)].append(k)

    def roles(boxes7):
        # boxes7: list of ((byte, bit), (x0, y0, x1, y1)) -> [T,UL,UR,M,LL,LR,B]
        wide = sorted([b for b in boxes7
                       if (b[1][2] - b[1][0]) >= 1.4 * (b[1][3] - b[1][1])],
                      key=lambda b: (b[1][1] + b[1][3]) / 2)
        tall = [b for b in boxes7
                if (b[1][3] - b[1][1]) >= 1.4 * (b[1][2] - b[1][0])]
        assert len(wide) == 3, f"wide: {len(wide)}"
        assert len(tall) == 4, f"tall: {len(tall)}"
        t, m, b = wide[0][0], wide[1][0], wide[2][0]
        cx = sorted((bb[1][0] + bb[1][2]) / 2 for bb in tall)
        mid_x = (cx[1] + cx[2]) / 2
        left = sorted([bb for bb in tall
                       if (bb[1][0] + bb[1][2]) / 2 < mid_x],
                      key=lambda bb: (bb[1][1] + bb[1][3]) / 2)
        right = sorted([bb for bb in tall
                        if (bb[1][0] + bb[1][2]) / 2 >= mid_x],
                       key=lambda bb: (bb[1][1] + bb[1][3]) / 2)
        assert len(left) == 2 and len(right) == 2
        return [t, left[0][0], right[0][0], m,
                left[1][0], right[1][0], b]

    score_ds, singles, big = [], {}, None
    for root, members in clusters.items():
        if len(members) == 7 and max(rest[k][1] for k in members) < 700:
            score_ds.append(members)
        elif len(members) == 1:
            singles.setdefault(members[0], root)
        elif len(members) == 7:
            pass  # speed/level digits handled below by known y-bands
        else:
            big = members
    assert len(score_ds) == 4, f"score digits: {len(score_ds)}"
    score_ds.sort(key=lambda ms: min(rest[k][0] for k in ms))
    score_roles = [roles([(k, rest[k]) for k in ms]) for ms in score_ds]

    def digit_at(y_lo, y_hi):
        cand = [k for k, v in rest.items()
                if y_lo <= (v[1] + v[3]) / 2 <= y_hi
                and (v[2] - v[0]) >= 15 and (v[3] - v[1]) >= 15]
        assert len(cand) == 7, f"digit y[{y_lo},{y_hi}]: {len(cand)}"
        return roles([(k, rest[k]) for k in cand])

    speed_roles = digit_at(1260, 1390)  # below SPEED label
    level_roles = digit_at(1480, 1610)  # above LEVEL label

    # 4x4 next-piece block: rows by y-band, col->bit (bit falls with x).
    next_rows = []
    for y_lo, y_hi in ((440, 530), (508, 598), (576, 666), (644, 734)):
        row = sorted([k for k, v in rest.items()
                      if y_lo <= (v[1] + v[3]) / 2 <= y_hi
                      and (v[0] + v[2]) / 2 >= 1440],
                     key=lambda k: (rest[k][0] + rest[k][2]) / 2)
        assert len(row) == 4, f"next row: {len(row)}"
        bits = [b for _, b in row]
        assert bits == [5, 4, 3, 2], f"next col->bit: {bits}"
        assert len({a for a, _ in row}) == 1
        next_rows.append(row)

    def one(byte, bit, label):
        assert (byte, bit) in rest, label
        return (byte, bit)

    hud = {
        "lead1": one(38, 1, "lead1"),
        "note": one(44, 1, "note"),
        "dot": one(14, 1, "dot"),
        "runner": one(15, 2, "runner"),
        "alarm": [one(15, 3, "alarm0"), one(15, 4, "alarm1")],
        "speed_label": one(15, 5, "speed_label"),
        "level_label": one(26, 1, "level_label"),
        "coffee": one(32, 1, "coffee"),
    }

    with open(OUT_PATH, "w") as f:
        f.write("// Auto-generated by tools/bake_segments.py -- DO NOT EDIT.\n")
        f.write("// Apollo 18 in 1 display-only segment map (237 segments).\n")
        f.write("#pragma once\n\n#include <array>\n#include <cstdint>\n\n")
        f.write("namespace brickemu {\n\n")
        f.write(f"inline constexpr std::uint16_t kApollo18CanvasW = {canvas_w};\n")
        f.write(f"inline constexpr std::uint16_t kApollo18CanvasH = {CANVAS_H};\n")
        f.write("inline constexpr std::uint8_t kApollo18LcdramSize = 0x30;\n\n")
        f.write("struct Apollo18Seg {\n"
                "    std::uint16_t x, y, w, h;\n"
                "    std::uint8_t byte, bit;\n};\n\n")
        f.write("// One rect per LCD segment, in LCD-canvas pixels.\n"
                "inline constexpr std::array<Apollo18Seg, 237> kApollo18Segs = {{\n")
        for x, y, w, h, byte, bit in entries:
            f.write(f"    {{{x}, {y}, {w}, {h}, {byte}, {bit}}},\n")
        f.write("}};\n\n")
        f.write("// Playfield cell map: original 10x17 LCD matrix, row-major,\n"
                "// row 0 = top, col 0 = left. One LCDRAM bit per cell.\n"
                "inline constexpr std::uint8_t kApollo18PfCols = 10;\n"
                "inline constexpr std::uint8_t kApollo18PfRows = 17;\n"
                "struct Apollo18PfCell {\n"
                "    std::uint8_t byte, bit;\n"
                "};\n"
                "inline constexpr std::array<Apollo18PfCell, 170> kApollo18Pf = {{\n")
        for row in range(17):
            cells = ", ".join(
                f"{{{pfmap[(row, col)][0]}, {pfmap[(row, col)][1]}}}"
                for col in range(10))
            f.write(f"    {cells},\n")
        f.write("}};\n\n")

        def segline(cells):
            return ", ".join(f"{{{a}, {b}}}" for a, b in cells)

        f.write("// HUD map: score digits (D0 = leading '1', D1..D4 full 7-seg),\n"
                "// speed + level digits, icons. Digit segment order: top,\n"
                "// upper-left, upper-right, mid, lower-left, lower-right, bottom.\n"
                "// (Flat tables: nested std::array init is brittle; index\n"
                "// [digit * 7 + role] / [row * 4 + col].)\n"
                "struct Apollo18SegBit {\n"
                "    std::uint8_t byte, bit;\n};\n"
                f"inline constexpr Apollo18SegBit kApollo18Lead1 = "
                f"{{{hud['lead1'][0]}, {hud['lead1'][1]}}};\n"
                "inline constexpr std::array<Apollo18SegBit, 28> "
                "kApollo18Score = {{")
        f.write(", ".join(segline(r) for r in score_roles))
        f.write("}};\n")
        f.write("inline constexpr std::array<Apollo18SegBit, 7> "
                "kApollo18Speed = {{" + segline(speed_roles) + "}};\n")
        f.write("inline constexpr std::array<Apollo18SegBit, 7> "
                "kApollo18Level = {{" + segline(level_roles) + "}};\n")
        for key in ("note", "dot", "runner", "coffee",
                    "speed_label", "level_label"):
            a, b = hud[key]
            f.write(f"inline constexpr Apollo18SegBit kApollo18_{key} = "
                    f"{{{a}, {b}}};\n")
        f.write("inline constexpr std::array<Apollo18SegBit, 2> "
                "kApollo18Alarm = {{" + segline(hud['alarm']) + "}};\n")
        f.write("// Next-piece 4x4 block, row-major (row 0 = top).\n"
                "inline constexpr std::array<Apollo18SegBit, 16> "
                "kApollo18Next = {{")
        f.write(", ".join(segline(r) for r in next_rows))
        f.write("}};\n\n} // namespace brickemu\n")
    print(f"wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
