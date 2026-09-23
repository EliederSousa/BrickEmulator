# Architecture Checklist
checklist:
  name: "architecture-checklist"
  version: "1.0"
  categories:
    - name: "Architectural Decisions"
      items:
        - "Architectural pattern selected and justified"
        - "Component boundaries defined"
        - "Interfaces are pure virtual (dependency inversion)"
        - "Data flow documented for all critical paths"
        - "Threading model defined and reviewed"
        
    - name: "Component Design"
      items:
        - "Each component has single responsibility"
        - "Components are loosely coupled"
        - "High cohesion within components"
        - "Interfaces are minimal and focused"
        - "Ownership semantics clear (unique/shared/weak)"
        - "Error handling strategy consistent"
        
    - name: "CPU Core Architecture"
      items:
        - "Common ICpuCore interface defined"
        - "Instruction decoding strategy decided"
        - "Cycle counting approach defined"
        - "Memory map abstraction designed"
        - "Interrupt handling designed"
        - "Disassembler interface separate from core"
        - "Core registration mechanism (factory/registry)"
        
    - name: "Peripheral Architecture"
      items:
        - "Interconnect pub/sub pattern defined"
        - "Device categories: Input, Clock, Serial, Port"
        - "Port I/O abstraction matches Python"
        - "Wakeup/interrupt mechanism designed"
        - "EEPROM persistence strategy"
        - "IR protocol handlers modular"
        
    - name: "Display Rendering"
      items:
        - "SVG parsing strategy (build-time vs runtime)"
        - "Segment mapping data structure"
        - "Motion blur algorithm ported"
        - "Ghost segments algorithm ported"
        - "Shadow effect implemented"
        - "OpenGL ES 2.0 shader approach"
        - "Software fallback for unsupported devices"
        
    - name: "Audio Engine"
      items:
        - "Tone generation algorithm ported"
        - "Noise generation ported"
        - "Mixing strategy (software vs hardware)"
        - "SDL2 audio callback integration"
        - "Buffer sizing for low latency"
        - "Channel management"
        
    - name: "Memory Management"
      items:
        - "No raw owning pointers"
        - "RAII for all resources"
        - "Object pools for hot allocations"
        - "Static allocation for fixed-size buffers"
        - "Custom allocator for emulator state"
        - "No exceptions in hot paths"
        - "Memory budget per component"
        
    - name: "Configuration & Assets"
      items:
        - ".brick JSON schema defined"
        - "Build-time asset processing pipeline"
        - "ROM loading strategy (embed vs filesystem)"
        - "Asset versioning"
        
    - name: "Debug Interface"
      items:
        - "Breakpoint mechanism designed"
        - "Step execution (single instruction)"
        - "State inspection API"
        - "Register/memory view"
        - "Disassembly view integration"
        
    - name: "Cross-Cutting Concerns"
      items:
        - "Logging strategy (compile-time levels)"
        - "Assertion strategy (debug vs release)"
        - "Profiling hooks"
        - "Deterministic execution for replay"