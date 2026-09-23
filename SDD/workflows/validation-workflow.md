# Validation Workflow
workflow:
  name: "validation-workflow"
  version: "1.0"
  description: "Validation and verification process for emulation accuracy"
  stages:
    - name: "Unit Level Validation"
      description: "Validate individual functions/classes"
      agent: "qa-engineer"
      tools:
        - "GoogleTest / Catch2"
        - "Property-based testing (RapidCheck)"
      criteria:
        - "All CPU instructions match reference implementation"
        - "Peripheral handlers produce expected outputs"
        - "Memory operations (read/write) match exactly"
        - "Timing calculations match Python"
      automation: "CI pipeline on every commit"
      
    - name: "Integration Level Validation"
      description: "Validate component interactions"
      agent: "qa-engineer"
      tools:
        - "Custom test harness"
        - "Instruction trace comparison"
      scenarios:
        - "Boot sequence for each core"
        - "Button press/release handling"
        - "Display update cycle"
        - "Audio tone generation"
        - "Serial/IR communication"
      criteria:
        - "Instruction trace matches Python (first N instructions)"
        - "VRAM state matches at each display frame"
        - "Audio samples match within tolerance"
        
    - name: "System Level Validation"
      description: "Full game validation"
      agent: "qa-engineer"
      tools:
        - "Automated gameplay scripts"
        - "Visual diff tool"
        - "Audio comparison"
      test_matrix:
        cores:
          - "HT943: E23PlusMarkII96in1, E88, GA878"
          - "SPLB32: Tamagotchi Connection V3"
          - "LC5732: Hiro Pocket Boy, Edla Space Rescue"
          - "E0C6200: Radio Shack Stack Challenge"
          - "KS57C21308: Pocket Puppy"
          - "SPLB20: Big Cat 9-in-1"
          - "GPL191X: Tamagotchi Mini 2017"
          - "STK55C324: Mini Classics Soccer"
          - "MSM50XX: Bandai Pengo"
          - "T6770S: Gundam Space Combat"
          - "T7741: Spica D'Artagnan"
          - "KS57C2504: Koto Laboratory"
          - "SPL02/SPL03: Apollo games"
          - "SPL81408: McDonald's Chicken Nugget"
          - "KS56CX2X: Formel 1"
          - "M37520: Epoch Monster Panic"
          - "EM73000: Various"
          - "D750X: Various"
          - "HTG12N0: Mickey Deluxe"
        scenarios_per_game:
          - "Cold boot to title screen"
          - "Navigate menus"
          - "Play 30 seconds gameplay"
          - "Save/load state"
          - "Toggle sound on/off"
      criteria:
        - "Visual output: < 0.1% pixel difference"
        - "Audio: < 1% sample difference"
        - "Performance: 60 FPS sustained"
        - "Memory: < 64MB"
        - "No crashes for 1 hour gameplay"
        
    - name: "Regression Validation"
      description: "Ensure no regressions after changes"
      agent: "qa-engineer"
      frequency: "Every PR + nightly"
      automation: "Full test matrix on CI"
      criteria:
        - "All previous validation passes"
        - "No new memory leaks"
        - "Performance within 5% of baseline"
        
    - name: "PortMaster Device Validation"
      description: "Validation on actual target hardware"
      agent: "devops-engineer"
      devices:
        - "RG35XX (ARM Cortex-A53)"
        - "RG353P (ARM Cortex-A55)"
        - "TrimUI Smart (ARM Cortex-A7)"
        - "Miyoo Mini Plus (ARM Cortex-A7)"
      criteria:
        - "Package installs correctly"
        - "Launches from PortMaster menu"
        - "Controls mapped correctly"
        - "Audio works via device speakers"
        - "Save states persist"
        - "Battery life impact acceptable"

  comparison_tools:
    - name: "Trace Comparator"
      description: "Compare instruction traces between Python and C++"
      input: "JSON instruction logs"
      output: "Diff report with divergence point"
      
    - name: "VRAM Comparator"
      description: "Frame-by-frame VRAM comparison"
      input: "VRAM dumps per frame"
      output: "Visual diff + difference percentage"
      
    - name: "Audio Comparator"
      description: "Compare audio output"
      input: "WAV files from both implementations"
      output: "Spectral diff + RMS difference"
      
    - name: "Performance Profiler"
      description: "Profile on target hardware"
      tool: "perf / VTune / simple FPS counter"
      metrics: ["FPS", "Frame time (min/avg/max)", "CPU usage", "Memory usage"]