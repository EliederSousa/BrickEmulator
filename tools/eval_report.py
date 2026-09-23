#!/usr/bin/env python3
"""Generate a self-contained HTML display evaluator (no dependencies).

Boots the real Python SPL03 core, plays a scripted button timeline, samples
LCDRAM + CPU state every emulated frame (11500 cycles @ 690 kHz = 60 fps),
and writes /tmp/opencode/eval_report.html: a canvas player (play/pause,
scrub, step) rendering the baked 237-segment Apollo LCD plus an input/state
log. Open it in Firefox.

Usage: python3 tools/eval_report.py [frames]   # default 90
"""
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

FRAMES = int(sys.argv[1]) if len(sys.argv) > 1 else 90
CYCLES_PER_FRAME = 11500  # 690000 Hz / 60 fps, in CPU *cycles* (one clock()
                          # call = one instruction of several cycles, so run
                          # to a cycle target, not a call count)

HDR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                   "portmaster-build", "include", "brickemu",
                   "Apollo18Segments.hpp")
hdrsrc = open(HDR).read()
W = int(re.search(r"kApollo18CanvasW = (\d+)", hdrsrc).group(1))
H = int(re.search(r"kApollo18CanvasH = (\d+)", hdrsrc).group(1))
SEGS = [tuple(int(g) for g in m.groups()) for m in
        re.finditer(r"\{(\d+), (\d+), (\d+), (\d+), (\d+), (\d+)\}", hdrsrc)]
assert (W, H) == (332, 480) and len(SEGS) == 237

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

# (frame_index, action, port, mask, level, label)
TIMELINE = [
    (5, "press", "PA", 1, 1, "OnOff press"),
    (10, "press", "PA", 1, -1, "OnOff release"),
    (15, "press", "PA", 64, 1, "Start press"),
    (20, "press", "PA", 64, -1, "Start release"),
    (30, "press", "PA", 2, 1, "Rotate press"),
    (33, "press", "PA", 2, -1, "Rotate release"),
    (40, "press", "PA", 8, 1, "Left press"),
    (45, "press", "PA", 8, -1, "Left release"),
    (50, "press", "PA", 16, 1, "Right press"),
    (55, "press", "PA", 16, -1, "Right release"),
    (60, "press", "PA", 4, 1, "Down press"),
    (65, "press", "PA", 4, -1, "Down release"),
    (70, "press", "PA", 2, 1, "Rotate press"),
    (73, "press", "PA", 2, -1, "Rotate release"),
]

frames = []
events = []
ti = 0


def run_cycles(n):
    target = cpu._cycle_counter + n
    while cpu._cycle_counter < target:
        cpu.clock()


for f in range(FRAMES):
    while ti < len(TIMELINE) and TIMELINE[ti][0] == f:
        _, _, port, mask, level, label = TIMELINE[ti]
        cpu.port_handler(port, mask, level)
        events.append({"frame": f, "label": label})
        ti += 1
    run_cycles(CYCLES_PER_FRAME)
    st = cpu.examine()
    frames.append({
        "vram": bytes(cpu.get_VRAM()).hex(),
        "pc": st["PC"], "a": st["A"], "x": st["X"], "sp": st["SP"],
        "flags": (int(bool(st["NF"])), int(bool(st["VF"])),
                  int(bool(st["DF"])), int(bool(st["BF"])),
                  int(bool(st["IF"])), int(bool(st["ZF"])),
                  int(bool(st["CF"]))),
    })
    if f % 15 == 0:
        print(f"  frame {f}/{FRAMES}", flush=True)

data = {"w": W, "h": H,
        "segs": SEGS,
        "frames": frames,
        "events": events,
        "cyclesPerFrame": CYCLES_PER_FRAME}

html = """<!DOCTYPE html>
<html><head><meta charset="utf-8"><title>BrickEmu Apollo18 display eval</title>
<style>
body{background:#222;color:#ddd;font-family:sans-serif;display:flex;gap:16px;
  justify-content:center;padding:16px}
#lcd{border:4px solid #111;background:#9ca66c;image-rendering:pixelated}
#panel{max-width:420px}
button,select,input{margin:2px;font-size:14px}
#log{max-height:300px;overflow-y:auto;background:#111;padding:8px;font-size:13px}
#log div{cursor:pointer;padding:1px 4px}#log div:hover{background:#333}
#state{font-family:monospace;background:#111;padding:8px;margin-top:8px}
</style></head><body>
<div><canvas id="lcd" width="332" height="480"></canvas></div>
<div id="panel">
<h2>Apollo 18 in 1 — display eval</h2>
<div>
<button id="play">Pause</button>
<button id="prev">|&#9664;</button>
<button id="next">&#9654;|</button>
<label>speed <select id="speed"><option>1</option><option>2</option>
<option selected>4</option><option>8</option></select>x</label>
</div>
<div><input id="scrub" type="range" min="0" max="FRAMESMAX" value="0"
  style="width:100%"></div>
<div id="state"></div>
<h3>input timeline (click to seek)</h3>
<div id="log"></div>
</div>
<script>
const D = DATAJSON;
const cv = document.getElementById('lcd'), cx = cv.getContext('2d');
let f = 0, playing = true, speed = 4, acc = 0, last = performance.now();
function draw(i) {
  const fr = D.frames[i];
  const vram = [];
  for (let k = 0; k < fr.vram.length; k += 2)
    vram.push(parseInt(fr.vram.substr(k, 2), 16));
  cx.fillStyle = '#9ca66c'; cx.fillRect(0, 0, D.w, D.h);
  cx.fillStyle = '#1e221a';
  for (const s of D.segs)
    if (s[4] < vram.length && (vram[s[4]] >> s[5]) & 1)
      cx.fillRect(s[0], s[1], s[2], s[3]);
  const fl = fr.flags;
  document.getElementById('state').textContent =
    'frame ' + i + '/' + (D.frames.length - 1) +
    '  pc=' + fr.pc.toString(16).padStart(4, '0') +
    ' A=' + fr.a.toString(16).padStart(2, '0') +
    ' X=' + fr.x.toString(16).padStart(2, '0') +
    ' SP=' + fr.sp.toString(16).padStart(2, '0') +
    ' N' + fl[0] + 'V' + fl[1] + 'D' + fl[2] + 'B' + fl[3] +
    'I' + fl[4] + 'Z' + fl[5] + 'C' + fl[6];
  document.getElementById('scrub').value = i;
}
const log = document.getElementById('log');
for (const e of D.events) {
  const d = document.createElement('div');
  d.textContent = 'f' + e.frame + ': ' + e.label;
  d.onclick = () => { f = e.frame; draw(f); };
  log.appendChild(d);
}
document.getElementById('play').onclick = (e) => {
  playing = !playing; e.target.textContent = playing ? 'Pause' : 'Play';
  last = performance.now();
};
document.getElementById('prev').onclick = () => {
  f = Math.max(0, f - 1); draw(f);
};
document.getElementById('next').onclick = () => {
  f = Math.min(D.frames.length - 1, f + 1); draw(f);
};
document.getElementById('scrub').oninput = (e) => { f = +e.target.value; draw(f); };
document.getElementById('speed').onchange = (e) => { speed = +e.target.value; };
function tick(t) {
  if (playing) {
    acc += (t - last) * 60 / 1000 * speed;
    last = t;
    if (acc >= 1) {
      f = (f + Math.floor(acc)) % D.frames.length;
      acc %= 1; draw(f);
    }
  } else last = t;
  requestAnimationFrame(tick);
}
draw(0); requestAnimationFrame(tick);
</script></body></html>"""
html = html.replace("FRAMESMAX", str(FRAMES - 1))
html = html.replace("DATAJSON", json.dumps(data))

os.makedirs("/tmp/opencode", exist_ok=True)
out = "/tmp/opencode/eval_report.html"
open(out, "w").write(html)
print(f"wrote {out} ({os.path.getsize(out) // 1024} KB, {FRAMES} frames)")
