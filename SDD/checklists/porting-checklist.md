# Porting Checklist - Python to C++ for PortMaster
checklist:
  name: "porting-checklist"
  version: "1.0"
  categories:
    - name: "Project Setup"
      items:
        - "CMakeLists.txt created with C++20"
        - "SDL2 dependency configured (FetchContent/find_package)"
        - "nlohmann/json dependency configured"
        - "lunasvg/nanosvg dependency configured"
        - "GoogleTest/Catch2 configured"
        - "Cross-compilation toolchains defined (arm-linux-gnueabihf, aarch64-linux-gnu)"
        - "Docker build environment created"
        - "CI/CD pipeline configured (GitHub Actions)"
        - "Clang-format configured"
        - "Clang-tidy configured"
        - "Static analysis (cppcheck) configured"
        
    - name: "Foundation Components"
      items:
        - "ConfigLoader: .brick JSON parsing"
        - "AssetLoader: ROM/SROM/SVG loading"
        - "Timing: High-resolution clock wrapper"
        - "MessageQueue: Lock-free SPSC/MPSC queues"
        - "Threading: Thread wrapper with affinity"
        - "Logging: Compile-time level logging"
        - "ErrorHandling: std::expected-based"
        - "Memory: Custom allocators, object pools"
        
    - name: "CPU Cores - Priority 1 (MVP)"
      items:
        - "HT943: Core + Disassembler"
        - "SPLB32: Core + Disassembler"
        - "LC5732: Core + Disassembler"
        - "E0C6200: Core + Disassembler"
        - "KS57C21308: Core + Disassembler"
        - "SPLB20: Core + Disassembler"
        
    - name: "CPU Cores - Priority 2"
      items:
        - "GPL191X: Core + Disassembler"
        - "STK55C324: Core + Disassembler"
        - "MSM50XX: Core + Disassembler"
        - "T6770S: Core + Disassembler"
        - "T7741: Core + Disassembler"
        - "KS57C2504: Core + Disassembler"
        - "SPL02/SPL03: Core + Disassembler"
        - "SPL81408: Core + Disassembler"
        - "KS56CX2X: Core + Disassembler"
        - "M37520: Core + Disassembler"
        - "EM73000: Core + Disassembler"
        - "D750X: Core + Disassembler"
        - "HTG12N0: Core + Disassembler"
        
    - name: "Peripherals"
      items:
        - "Interconnect: Pub/sub message bus"
        - "DirectInput: Port-mapped buttons"
        - "MatrixInput: Matrix-scanned buttons"
        - "DirectConnection: Direct port connections"
        - "HT24LC08: EEPROM emulation"
        - "CON_V3_IR: Tamagotchi V3 IR"
        - "CON_DGM: Digimon IR"
        
    - name: "Display Rendering"
      items:
        - "SvgParser: Build-time SVG to segment map"
        - "SegmentRenderer: OpenGL ES 2.0"
        - "MotionBlur: Exponential smoothing"
        - "GhostSegments: Persistence effect"
        - "ShadowEffect: Offset duplicate rendering"
        - "ButtonOverlay: Clickable SVG elements"
        - "SoftwareFallback: CPU rendering path"
        
    - name: "Audio Engine"
      items:
        - "ToneGenerator: Sine wave + noise"
        - "Mixer: Multi-channel software mixing"
        - "AudioSink: SDL2 audio callback"
        - "ChannelManager: Start/stop/volume"
        - "SoundROM: Waveform table loading"
        
    - name: "Emulator Core"
      items:
        - "EmulatorCore: Main orchestration"
        - "MainLoop: Fixed timestep with frame pacing"
        - "StateMachine: Run/Pause/Stop/Step"
        - "BreakpointManager: Address-based breakpoints"
        - "SaveState: Serialize/deserialize full state"
        - "SpeedControl: 0.5x/1x/2x/4x/max"
        
    - name: "Debug Interface"
      items:
        - "DisassemblyView: Instruction listing"
        - "RegisterView: CPU register display"
        - "MemoryView: Hex dump with navigation"
        - "BreakpointUI: Set/clear/list breakpoints"
        - "StepControl: Step into/over/out"
        
    - name: "Serial/IR Communication"
      items:
        - "SerialPort: Cross-platform serial (SDL2/termios)"
        - "BluetoothSerial: RFCOMM support"
        - "IRTransceiver: GPIO/serial IR"
        - "ProtocolHandlers: V3, DGM protocols"
        
    - name: "PortMaster Integration"
      items:
        - "PortMaster control.txt generated"
        - "Executable entry point"
        - "Asset packaging (ROMs, SVGs, configs)"
        - "Icon and metadata"
        - "Controller mapping (gamepad to keys)"
        - "Fullscreen/windowed handling"
        - "Save directory handling"
        
    - name: "Optimization"
      items:
        - "Profile on target hardware (perf)"
        - "ARM NEON intrinsics for hot paths"
        - "Cache-friendly data layouts"
        - "Branch prediction hints"
        - "Link-time optimization (LTO)"
        - "Strip symbols in release"
        - "Startup time optimization"
        
    - name: "Validation"
      items:
        - "Instruction trace comparison (all cores)"
        - "VRAM frame comparison (all games)"
        - "Audio output comparison"
        - "Performance benchmarks (all games)"
        - "Memory leak testing (valgrind)"
        - "Stress testing (1 hour per game)"
        - "PortMaster device testing (4+ devices)"