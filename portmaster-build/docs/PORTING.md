# BrickEmuPy PortMaster Port

This directory is the C++ porting target described by `../SDD/`.

## Current Status

- CMake C++20 scaffold exists (`brickemu` CLI core + optional `brickemu-sdl`).
- Initial `.brick` config loader extracts `id`, `core`, `face_path`, `clock`, ROM paths, and display settings.
- Runtime interfaces are in place for CPU cores, peripherals, display, and audio.
- ROM loading is ported with the same modulo-addressed behavior as `cores/rom.py`.
- `SPL03` runs the Apollo 18 in 1 ROM with 200k-step exact trace parity vs Python
  (registers, flags, PC, cycle counters; see `testbed/RESULTS.md`).
- Display-only segment map for Apollo 18 in 1 is baked offline
  (`tools/bake_segments.py` -> `include/brickemu/Apollo18Segments.hpp`,
  237 rects, ~2.4 KB, zero SVG parsing on device).
- SDL-free rasterizer (`DisplayBuffer.hpp`, unit-tested in `testbed/test_display.cpp`).
- SDL2 frontend (`Frontend_SDL.cpp`): 640x480, logical LCD scaling, dirty-checked
  RGB565 upload, 690 kHz-paced loop, keyboard + gamecontroller mapped to PA-port
  buttons via `ICpuCore::setPortInput` (Python `port_handler` parity incl. RES
  reset and power-key NMI).
- `DirectInput` semantics live in the core port handler; audio (`SPL0Xsound`
  port) is the next slice.

## Rendering Direction (PortMaster standard: SDL2)

Per PortMaster practice the port uses the firmware's own KMSDRM-patched SDL2
for display/input (no X11/Wayland/GL required, no bundled libSDL2).
`brickemu-sdl --brick <game>.brick --run` is the handheld entry point;
`BRICKEMU_DUMPFRAME=<path.bmp>` (+ optional `BRICKEMU_DUMPFRAME_AT=N`) dumps
a frame for headless verification.

## Rendering Direction

PortMaster builds should render only the LCD/display layer by default. The Python SVG console skins/frames are useful on desktop, but they add parsing and draw cost on handhelds and are not needed for a clean PortMaster UI.

The display pipeline should therefore use `.brick`/core LCD RAM mappings and either:

- a compact display-only asset generated from the existing SVG segment IDs, or
- a hand-authored per-game segment map when generation is unreliable.

## Local Build

```bash
cmake -S portmaster-build -B portmaster-build/build
cmake --build portmaster-build/build
./portmaster-build/build/brickemu --brick assets/E23PlusMarkII96in1.brick
```

SDL2 frontend (needs `libsdl2-dev` on the build machine only):

```bash
./portmaster-build/build/brickemu-sdl --brick assets/Apollo18in1B0302.brick --run
```

ARM handheld target (e.g. RG35XX H; build on Ubuntu 20.04 for firmware glibc
compatibility, runtime SDL2 comes from the firmware):

```bash
cmake -S portmaster-build -B portmaster-build/build-arm \
  --toolchain portmaster-build/cmake/aarch64-linux-gnu.cmake
cmake --build portmaster-build/build-arm
```

## Next Porting Slice

1. Replace the temporary regex config reader with a real JSON parser once dependencies are selected.
2. Implement SPL0X/SPL03 instruction execution using `cores/SPL0X.py` as reference.
3. Add direct input mapping for Apollo 18 in 1 (`PA` bits and `RES`).
4. Add display-only segment map generation for Apollo 18 in 1.
5. Add an instruction trace harness to compare Python and C++ execution.
