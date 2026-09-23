# BrickEmuPy PortMaster Port - Session Summary

## Project Overview
Porting the Python PyQt6 BrickEmuPy emulator to C++ for PortMaster (ARM Linux handhelds). Target: Apollo 18 in 1 (SPL03 core) first, then other cores.

## What Was Completed

### 1. SDD/BMAD Framework (`SDD/`)
- Complete Spec Driven Development structure with agents, templates, workflows, checklists
- Feature inventory of 26 CPU cores, 6 peripherals, 70+ games
- Risk assessment with 15 identified risks and mitigations

### 2. C++ Project Scaffold (`portmaster-build/`)
```
portmaster-build/
├── CMakeLists.txt
├── include/brickemu/
│   ├── Config.hpp      # .brick config parsing
│   ├── Rom.hpp         # ROM loader (modulo addressing, byte/word reads)
│   ├── EmulatorRuntime.hpp  # Core interfaces
│   ├── CoreRegistry.hpp     # Factory for CPU cores
│   └── SPL03.hpp      # SPL03 core interface
├── src/
│   ├── Config.cpp     # Regex-based JSON parser
│   ├── Rom.cpp        # Binary ROM loading
│   ├── CoreRegistry.cpp
│   ├── EmulatorRuntime.cpp
│   ├── SPL03.cpp      # SPL03/SPL0X CPU core (in progress)
│   └── main.cpp       # CLI entry point
└── docs/PORTING.md    # Porting notes
```

### 3. Config & ROM Loading ✅
- `.brick` JSON parsing: id, core, face_path, clock, mask_options, buttons, display
- Apollo 18 mask options: `non_crystal_div`, `port_pullup`, `port_pkey`
- ROM loader with Python-compatible behavior (modulo addressing, little-endian word reads)

### 4. SPL03 Core Implementation (Partial) ✅
- Memory map: LCDRAM 0x00-0x2F, CPURAM 0x30-0x7F, SFR 0xC0-0xFF, ROM 0x1000+
- Registers: A, X, SP, PC, status flags (N,V,B,D,I,Z,C)
- Reset vector at 0x1FFC, NMI vector at 0x1FFA
- Timer system: T2Hz/T256Hz with NMI trigger via SFR 0xD2
- SFR handling: 0xD0 (system ctrl), 0xD2 (interrupt config)

### 5. Implemented Opcodes (37 total used by Apollo 18)
| Opcode | Instruction | Status |
|--------|-------------|--------|
| A2 | LDX #imm | ✅ |
| 9A | TXS | ✅ |
| 20 | JSR abs | ✅ |
| A9 | LDA #imm | ✅ |
| 85 | STA zp | ✅ |
| 60 | RTS | ✅ |
| 78 | SEI | ✅ |
| 4C | JMP abs | ✅ |
| E6 | INC zp | ✅ |
| D0 | BNE | ✅ |
| F0 | BEQ | ✅ |
| 29 | AND #imm | ✅ |
| 25 | AND zp | ✅ |
| 35 | AND zp,X | ✅ |
| 48 | PHA | ✅ |
| 68 | PLA | ✅ |
| 40 | RTI | ✅ |
| 08 | PHP | ✅ |
| 86 | STX zp | ✅ |
| A5 | LDA zp | ✅ |
| A6 | LDX zp | ✅ |
| C6 | DEC zp | ✅ |
| 05 | ORA zp | ✅ |
| 09 | ORA #imm | ✅ |
| 18 | CLC | ✅ |
| 2A | ROL A | ✅ |
| 49 | EOR #imm | ✅ |
| 65 | ADC zp | ✅ |
| 69 | ADC #imm | ✅ |
| 81 | STA (ind,X) | ✅ |
| 90 | BCC | ✅ |
| 95 | STA zp,X | ✅ |
| A1 | LDA (ind,X) | ✅ |
| B0 | BCS | ✅ |
| BD | LDA abs,X | ✅ |
| C5 | CMP zp | ✅ |
| C9 | CMP #imm | ✅ |
| E0 | CPX #imm | ✅ |
| E8 | INX | ✅ |

### 6. Verification Status
- ✅ First 5,000 instructions: **EXACT MATCH** with Python reference
- ✅ First 50,000 instructions: 11 mismatches at step ~43,920
- ❌ Cycle counting: Python uses variable cycles per instruction, C++ mostly correct but needs verification
- ❌ PC diverges at step 43,960 (C++ executes wrong opcode 0x34 vs Python 0x20)

## Current Blocking Issue

### Divergence at Step 43,920
The A register diverges first at step 43,920:
- **Python**: A=0x6B at PC=06D5 (after BD LDA abs,X)
- **C++**: A=0x67 at same PC

**Root cause suspected**: BD (LDA abs,X) page-crossing penalty or effective address calculation differs from Python.

**Python BD at 06D2**: `BD 56 04` → base=0x0456, X=0x14 → addr=0x046A → reads 0x6B
**C++ BD at 06D2**: Same base/X but reads 0x67

Need to verify:
1. ROM byte at 0x046A (should be 0x6B per Python)
2. C++ `readMem()` mapping for address 0x046A (should hit ROM directly since < 0x1000)
3. Page crossing cycle penalty logic (adds 1 cycle but shouldn't affect value)

## Pending Tasks

### Immediate (This Step)
1. **Debug BD instruction**: Add detailed logging at step 43,920 to compare effective address and ROM byte read
2. **Verify ROM byte at 0x046A** in C++ matches Python (0x6B)
4. **Fix cycle counter** to match Python's variable cycles per instruction exactly
5. **Run full 200k trace comparison** to ensure zero mismatches

### Next Steps After Apollo 18
1. **DirectInput peripheral** for Apollo 18 buttons (PA port bits)
2. **Display rendering**: Display-only segment map (no SVG console frames)
3. **Audio engine**: SPL0Xsound port (4 channels: toneA, toneB, noise, speech)
4. **Save/load state** serialization
6. **PortMaster packaging**: control.txt, icon, controller mapping

### Remaining Cores Priority
- MVP (6): HT943, SPLB32, LC5732, E0C6200, KS57C21308, SPLB20
- Phase 2 (8): GPL191X, STK55C324, MSM50XX, T6770S, T7741, KS57C2504, SPL02/SPL03, SPL81408
- Phase 3 (8): KS56CX2X, M37520, EM73000, D750X, HTG12N0, SPL61X, HT4BIT, SPLB32sound...

## Key Files to Reference
| File | Purpose |
|------|---------|
| `cores/SPL0X.py` | Python reference implementation |
| `cores/rom.py` | Python ROM class |
| `assets/Apollo18in1B0302.brick` | Config for Apollo 18 |
| `portmaster-build/src/SPL03.cpp` | C++ core under development |
| `portmaster-build/docs/PORTING.md` | Porting decisions |

## Build & Test Commands
```bash
# Build
g++ -std=c++20 -Wall -Wextra -Wpedantic -Iportmaster-build/include \
  portmaster-build/src/Config.cpp \
  portmaster-build/src/CoreRegistry.cpp \
  portmaster-build/src/EmulatorRuntime.cpp \
  portmaster-build/src/Rom.cpp \
  portmaster-build/src/SPL03.cpp \
  portmaster-build/src/main.cpp \
  -o /tmp/opencode/brickemu

# Test with trace
/tmp/opencode/brickemu --brick assets/Apollo18in1B0302.brick --steps 50000 --trace

# Quick verify first 5000 steps
python3 -c "
import json, subprocess
from cores.SPL03 import SPL03
# ... (trace comparison script from session)
"
```

## Session Context for Continuation
- Working directory: `/home/elieder/Downloads/BrickEmulator`
- Python reference in `cores/SPL03.py` (inherits from `SPL0X.py`)
- ROM at `assets/Apollo18in1B0302.bin` (12,288 bytes)
- All 37 opcodes implemented but BD has subtle bug causing divergence at ~44k steps
- Cycle counting needs validation against Python's `_cycle_counter`