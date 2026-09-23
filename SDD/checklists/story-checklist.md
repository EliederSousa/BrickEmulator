# Story Checklist
checklist:
  name: "story-checklist"
  version: "1.0"
  categories:
    - name: "Story Definition"
      items:
        - "Unique ID assigned"
        - "Title is descriptive"
        - "User story format correct"
        - "Epic parent identified"
        - "Phase assigned"
        - "Priority set"
        - "Estimate provided"
        
    - name: "Acceptance Criteria"
      items:
        - "Written in Gherkin (Given/When/Then)"
        - "Criteria are testable"
        - "Criteria cover happy path"
        - "Criteria cover edge cases"
        - "Criteria cover error conditions"
        - "Performance criteria included if applicable"
        
    - name: "Technical Preparation"
      items:
        - "Python reference files identified"
        - "Key classes/functions mapped"
        - "Algorithms documented"
        - "Data structures designed"
        - "Interfaces defined (header first)"
        - "Dependencies resolved"
        
    - name: "Implementation"
      items:
        - "Code follows project style guide"
        - "RAII used throughout"
        - "No memory leaks (valgrind clean)"
        - "No undefined behavior"
        - "Const-correctness maintained"
        - "Thread safety documented"
        - "Error handling via std::expected"
        - "Unit tests for all public functions"
        - "Unit tests cover edge cases"
        - "Code compiles with -Wall -Wextra -Werror"
        - "Code compiles on ARM32 and ARM64"
        
    - name: "Integration"
      items:
        - "Component integrates with message bus"
        - "Timing integration verified"
        - "Config loading works"
        - "Asset loading works"
        - "No circular dependencies"
        
    - name: "Validation"
      items:
        - "Accuracy validated vs Python reference"
        - "Performance meets NFR targets"
        - "Integration tests pass"
        - "Regression tests pass"
        - "Memory usage within budget"
        
    - name: "Documentation"
      items:
        - "Header files documented (Doxygen)"
        - "README updated if needed"
        - "Architecture decisions recorded"
        - "Known limitations documented"
        
    - name: "Review & Merge"
      items:
        - "Self-review completed"
        - "Peer review completed"
        - "CI pipeline passes"
        - "No linting errors"
        - "Merged to main branch"