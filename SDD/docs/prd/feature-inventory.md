# Feature Inventory - BrickEmuPy Python Codebase Analysis
version: "1.0"
date: "2024-01-15"
analyst: "business-analyst"

## Core Emulation Features

### CPU Cores (26 cores implemented)
| Core | File | Disassembler | Key Features |
|------|------|--------------|--------------|
| HT943 | HT943.py | HT943dasm.py | 4-bit, 1MHz, Brick Game standard |
| SPLB32 | SPLB32.py | SPLB32dasm.py | 32-bit, Tamagotchi Connection |
| LC5732 | LC5732.py | LC5732dasm.py | Sanyo 4-bit, virtual pets |
| E0C6200 | E0C6200.py | E0C6200dasm.py | Seiko Epson, Stack Challenge |
| KS57C21308 | KS57C21308.py | KS57C21308dasm.py | Samsung, Pocket Puppy |
| SPLB20 | SPLB20.py | SPLB20dasm.py | Big Cat 9-in-1 |
| GPL191X | GPL191X.py | GPL191Xdasm.py | Tamagotchi Mini 2017 |
| STK55C324 | STK55C324.py | STK55C324dasm.py | Mini Classics Soccer |
| MSM50XX | MSM50XX.py | MSM50XXdasm.py | OKI, Bandai Pengo |
| T6770S | T6770S.py | T6770Sdasm.py | Toshiba, Gundam |
| T7741 | T7741.py | T7741dasm.py | Toshiba, Spica D'Artagnan |
| KS57C2504 | KS57C2504.py | KS57C2504dasm.py | Samsung, Koto Lab |
| SPL02 | SPL02.py | SPL0Xdasm.py | Sunplus, Apollo 126in1 |
| SPL03 | SPL03.py | SPL0Xdasm.py | Sunplus, Apollo 18in1 |
| SPL81408 | SPL81408.py | SPL81408dasm.py | Sunplus, McDonald's |
| KS56CX2X | KS56CX2X.py | KS56CX2Xdasm.py | Formel 1 |
| M37520 | M37520.py | M37520dasm.py | Mitsubishi, Monster Panic |
| EM73000 | EM73000.py | EM73000dasm.py | Elan, various |
| D750X | D750X.py | D750Xdasm.py | NEC, various |
| HTG12N0 | HTG12N0.py | HTG12N0dasm.py | Holtek, Mickey Deluxe |
| SPL61X | (uses GPL191X) | GPL191Xdasm.py | D-Power Digivice |
| HT4BIT | HT4BIT.py | HT4BITdasm.py | Holtek 4-bit base |
| SPLB32sound | SPLB32sound.py | - | Sound for SPLB32 |
| LC5732sound | LC5732sound.py | - | Sound for LC5732 |
| E0C6200sound | E0C6200sound.py | - | Sound for E0C6200 |
| PinTogglingSound | PinTogglingSound.py | - | Generic pin sound |

### Peripherals (6 types)
| Peripheral | File | Purpose |
|------------|------|---------|
| MatrixInput | matrix_input.py | Matrix-scanned keypad |
| DirectInput | direct_input.py | Direct port-mapped buttons |
| DirectConnection | direct_connection.py | Port-to-port connections |
| HT24LC08 | HT24LC08.py | I2C EEPROM emulation |
| CON_V3_IR | CON_V3_IR.py | Tamagotchi V3 IR protocol |
| CON_DGM | CON_DGM.py | Digimon IR protocol |

### Interconnect System
- Pub/sub message bus (interconnect.py)
- Port I/O routing
- Clock distribution
- Audio callback routing
- Serial TX/RX routing

### Display Rendering (brick_widget.py)
- SVG face parsing (QtSvg)
- Segment mapping: {byte}_{bit} naming (256×8 = 2048 segments max)
- Motion blur (exponential smoothing, configurable)
- Ghost segments (persistence effect)
- Shadow effect (offset duplicate)
- Button overlays from SVG elements
- Zoom/pan with mouse wheel
- Fullscreen support

### Audio Engine (audio_engine.py)
- Software tone generation (sine + noise)
- Multi-channel mixing (Qt Multimedia QAudioSink)
- 44.1kHz, 16-bit, stereo
- Per-channel frequency/noise/amplitude
- Envelope control (attack/release via queue)

### Configuration System (.brick files)
JSON structure:
- id: UUID
- core: core name (maps to cores_map)
- face_path: SVG file
- clock: Hz (typically 1,000,000)
- buttons: hotkey mappings
- display: motion_blur, ghost_segments, shadow
- peripherals: peripheral configurations
- mask_options: ROM paths, port config, sound tables

### Debug Interface (debug_widget.py)
- Disassembly view
- Register inspection
- Breakpoint management (set/clear/list)
- Step execution (single instruction)
- Run/Pause/Stop/Step controls
- Memory examination

### Serial/IR Communication
- SerialConnection: Qt SerialPort wrapper
- USB/Bluetooth serial support
- IR transceiver integration
- Data routing through interconnect

### Multiprocessing Architecture
- EmulatorProcess in separate process (spawn method)
- Command queue (main → emulator)
- Data queue (emulator → main)
- QueueReaderThread for async data handling
- Windows timer resolution hack (timeBeginPeriod)

### Games Supported (70+ games)
- 17 Brick Game variants
- 24 Virtual Pet variants  
- 29 Other LCD games
- Many distributed without ROM (legal)

## Porting Priority Matrix

### MVP (Must Have - 6 cores, ~20 games)
1. HT943 - Most Brick Games
2. SPLB32 - Tamagotchi Connection
3. LC5732 - Virtual pets
4. E0C6200 - Stack Challenge
5. KS57C21308 - Pocket Puppy
6. SPLB20 - Big Cat 9-in-1

### Phase 2 (High Value - 8 cores)
7. GPL191X - Tamagotchi Mini
8. STK55C324 - Mini Classics
9. MSM50XX - Bandai games
10. T6770S - Gundam
11. T7741 - Spica
12. KS57C2504 - Koto Lab
13. SPL02/SPL03 - Apollo
14. SPL81408 - McDonald's

### Phase 3 (Completeness - 8 cores)
15. KS56CX2X
16. M37520
17. EM73000
18. D750X
19. HTG12N0
20. HT4BIT (base)
21. SPL61X (uses GPL191X)
22. Sound modules

## Technical Debt / Risks Identified
1. **Multiprocessing → Threading**: Major architectural change
2. **QtSvg → Custom SVG Parser**: No Qt in target
3. **Qt Multimedia → SDL2 Audio**: Different API
4. **Python dict/list → C++ containers**: Performance critical
5. **Dynamic typing → Static types**: Core/peripheral interfaces
6. **Cycle-accurate timing**: Must match Python exactly
7. **Sound ROM formats**: Undocumented, reverse-engineered
8. **Some cores high error probability**: V3, Mini 2017 dumps