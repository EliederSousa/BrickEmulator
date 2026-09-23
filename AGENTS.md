# BrickEmuPy - Agent Instructions

## Project Overview
Python PyQt6 emulator for LCD handheld games (Brick Game, Tamagotchi, etc.). Each game runs on a specific microcontroller core.

## Running the Emulator
```bash
# Windows
python -m pip install pyqt6
python main.py

# Linux
sudo apt update && sudo apt install python3-pip libxcb-cursor0
pip install pyqt6 --break-system-packages
python3 main.py
```

## Architecture
- **main.py**: Entry point, creates QApplication, loads stylesheet, shows main window
- **ui.py**: `Window` class - main window, loads `ui/main_window.ui`, manages brick configs, settings, serial ports
- **brick_widget.py**: `BrickWidget` - QGraphicsView for rendering, spawns `EmulatorProcess` in separate process, communicates via multiprocessing queues
- **emulator_process.py**: `EmulatorProcess` - runs in separate process, emulates CPU + peripherals at configured clock speed
- **cores/**: CPU cores + disassemblers (HT943, SPLB32, LC5732, E0C6200, KS57C21308, etc.) mapped in `cores/__init__.py:cores_map`
- **peripherals/**: Input (matrix/direct), EEPROM (HT24LC08), IR (CON_V3_IR, CON_DGM) mapped in `peripherals/__init__.py:peripherals_map`
- **interconnect.py**: Pub/sub bus connecting CPU, peripherals, audio, serial
- **audio_engine.py**: Qt Multimedia audio sink, software mixing

## Key Conventions
- **Multiprocessing**: Emulator runs in separate process (`spawn` start method). Communication via `multiprocessing.Queue` (cmd queue → emulator, data queue ← emulator)
- **Config (.brick files)**: JSON with `id`, `core`, `face_path` (SVG), `clock` (Hz), `buttons` (hotkeys), `display` settings, `peripherals`, `mask_options` (ROM paths, port config, sound tables)
- **SVG faces**: Segment elements named `{byte}_{bit}` (0-255, 0-7), button elements match `buttons` keys, "body" and "overlay" for static graphics
- **No tests/lint/typecheck**: Not configured in repo

## Common Tasks
- **Add new game**: Create `.brick` config in `assets/`, add ROM (`.bin`), sound ROM (`.srom`), SVG face. Reference existing configs.
- **Add new core**: Implement CPU class + disassembler in `cores/`, register in `cores/__init__.py`
- **Add peripheral**: Implement in `peripherals/`, register in `peripherals/__init__.py:peripherals_map`

## Debugging
- CLI args: `-brick <path>` to load specific config, `-bp <addr>...` for breakpoints
- Debug widget shows disassembly, register state, breakpoints
- F11 / Ctrl+Meta+F = fullscreen, Esc = exit fullscreen