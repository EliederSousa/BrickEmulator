# Spec Driven Development Workflow
workflow:
  name: "spec-driven-development"
  version: "1.0"
  description: "BMAD SDD workflow for BrickEmuPy port"
  phases:
    - name: "Phase 0: Discovery"
      agent: "business-analyst"
      duration: "1 week"
      activities:
        - "Analyze Python codebase completely"
        - "Document all cores, peripherals, configurations"
        - "Create feature inventory"
        - "Identify porting risks"
      deliverables:
        - "docs/prd/feature-inventory.md"
        - "docs/prd/risk-assessment.md"
      gates:
        - "Feature inventory complete"
        - "Risks documented and rated"
        
    - name: "Phase 1: Requirements & Planning"
      agent: "business-analyst"
      duration: "1 week"
      activities:
        - "Write PRD using template"
        - "Define user stories with acceptance criteria"
        - "Prioritize with product-manager"
        - "Define MVP scope"
      deliverables:
        - "docs/prd/requirements.md"
        - "docs/prd/user-stories.md"
        - "docs/prd/mvp-definition.md"
      gates:
        - "PRD approved by stakeholders"
        - "MVP scope agreed"
        
    - name: "Phase 2: Architecture Design"
      agent: "architect"
      duration: "1 week"
      activities:
        - "Create system architecture document"
        - "Define component interfaces"
        - "Select libraries (SDL2, JSON, SVG)"
        - "Design threading model"
        - "Plan memory layout"
      deliverables:
        - "docs/architecture/system-design.md"
        - "docs/architecture/tech-stack.md"
        - "docs/architecture/porting-strategy.md"
      gates:
        - "Architecture review passed"
        - "Tech stack validated on target"
        
    - name: "Phase 3: Sprint Planning"
      agent: "product-manager"
      duration: "Ongoing"
      activities:
        - "Break epics into stories"
        - "Estimate story points"
        - "Create sprint backlog"
        - "Assign to developers"
      deliverables:
        - "docs/prd/sprint-backlog.md"
        - "docs/prd/release-plan.md"
      gates:
        - "Sprint backlog prioritized"
        - "Capacity planned"
        
    - name: "Phase 4: Implementation Sprints"
      agent: "developer"
      duration: "Multiple 2-week sprints"
      activities:
        - "Implement stories per sprint"
        - "Write unit tests"
        - "Continuous integration"
        - "Daily standups"
      deliverables:
        - "portmaster-build/src/"
        - "portmaster-build/tests/"
      gates:
        - "Sprint demo passes acceptance criteria"
        - "CI pipeline green"
        
    - name: "Phase 5: Validation & QA"
      agent: "qa-engineer"
      duration: "1 week per major release"
      activities:
        - "Functional testing vs Python reference"
        - "Performance benchmarking"
        - "Regression testing"
        - "Accuracy validation"
      deliverables:
        - "docs/qa/test-report.md"
        - "docs/qa/benchmark-results.md"
      gates:
        - "All MVP games pass accuracy tests"
        - "Performance targets met"
        
    - name: "Phase 6: PortMaster Packaging"
      agent: "devops-engineer"
      duration: "1 week"
      activities:
        - "Create PortMaster package"
        - "Cross-compile for ARM32/ARM64"
        - "Test on target hardware"
        - "Submit to PortMaster repo"
      deliverables:
        - "PortMaster .zip package"
        - "Installation scripts"
      gates:
        - "Package installs and runs on device"
        - "PortMaster maintainers accept"