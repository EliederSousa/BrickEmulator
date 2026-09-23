# Risk Assessment - BrickEmuPy Port to PortMaster
version: "1.0"
analyst: "business-analyst"

## Risk Categories

### Technical Risks

| ID | Risk | Probability | Impact | Mitigation |
|----|------|-------------|--------|------------|
| T-01 | Multiprocessing to threading architecture change breaks timing | High | Critical | Design lock-free message passing first; validate with instruction trace comparison |
| T-02 | Cycle-accurate emulation not achievable in C++ due to compiler optimizations | Medium | Critical | Use volatile/atomic for timing-critical vars; disable LTO for emulator core; verify with trace diff |
| T-03 | SVG parsing at runtime too slow on ARM Cortex-A7 | Medium | High | Pre-process SVG at build time to binary segment map; benchmark both approaches |
| T-04 | Audio latency too high with SDL2 callback | Low | Medium | Use small buffer sizes (512-1024 samples); test on target hardware early |
| T-05 | Memory usage exceeds 64MB on complex games | Medium | High | Profile memory per component; use object pools; static allocation for fixed buffers |
| T-06 | Some CPU cores have undocumented instructions/behavior | High | Medium | Focus on instruction trace validation; document deviations; accept minor inaccuracies for low-priority games |
| T-07 | Sound ROM formats not fully understood | Medium | Medium | Reverse-engineer from Python implementation; create test vectors |
| T-08 | PortMaster packaging requirements change | Low | Medium | Follow PortMaster docs closely; test packaging early; automate |

### Schedule Risks

| ID | Risk | Probability | Impact | Mitigation |
|----|------|-------------|--------|------------|
| S-01 | CPU core porting takes longer than estimated | High | High | Parallelize core porting; start with HT943 as reference; timebox each core |
| S-02 | Validation/debugging cycle-accuracy takes longer | High | High | Invest in automated comparison tools early; trace diff, VRAM diff, audio diff |
| S-03 | Target hardware not available for testing | Medium | High | Use QEMU user-mode for ARM32/ARM64 CI; borrow/buy devices early |
| S-04 | PortMaster acceptance process delays | Low | Medium | Engage PortMaster community early; follow existing port examples |

### Quality Risks

| ID | Risk | Probability | Impact | Mitigation |
|----|------|-------------|--------|------------|
| Q-01 | Emulation accuracy insufficient for gameplay | Medium | Critical | Automated trace/VRAM/audio comparison on every commit; gate merges on accuracy |
| Q-02 | Performance regression after optimization | Medium | High | Continuous benchmarking in CI; performance budgets per component |
| Q-03 | Memory leaks in long-running emulation | Low | High | Valgrind/ASan in CI; 1-hour stress tests per game |
| Q-04 | Save state corruption | Medium | Medium | Versioned save state format; round-trip testing; corruption detection |

### Dependency Risks

| ID | Risk | Probability | Impact | Mitigation |
|----|------|-------------|--------|------------|
| D-01 | SDL2 version mismatch on target devices | Low | Medium | Static link SDL2; test on multiple PortMaster devices |
| D-02 | lunasvg/nanosvg license/compatibility issues | Low | Low | Verify licenses (both MIT/BSD); have fallback parser |
| D-03 | nlohmann/json compile time too slow | Low | Low | Use single-header; precompiled headers; or switch to simdjson |

## Risk Mitigation Strategy

### Phase 0: Risk Reduction (Week 1-2)
1. Build timing comparison harness (Python ↔ C++)
2. Prototype lock-free message queue
3. Benchmark SVG parsing approaches
4. Test SDL2 audio on target hardware
5. Set up cross-compilation CI

### Phase 1: Core Porting with Validation (Week 3-8)
1. Port HT943 first (most games, best documented)
2. Validate every commit with trace comparison
3. Build automated VRAM/audio comparison
4. Establish performance baselines

### Phase 2: Continuous Validation
1. Nightly full game matrix on CI
2. Weekly device testing
3. Performance regression detection
4. Accuracy regression detection

## Go/No-Go Criteria for MVP Release

### Must Pass (No Exceptions)
- [ ] All 6 MVP cores pass instruction trace comparison (10k instructions)
- [ ] All 20 MVP games reach gameplay at 60 FPS on Cortex-A7
- [ ] Memory < 64MB for all MVP games
- [ ] No crashes in 1-hour stress test per game
- [ ] PortMaster package accepted

### Should Pass (Target)
- [ ] Audio matches within 1% RMS difference
- [ ] Visual output < 0.1% pixel difference
- [ ] Startup < 2 seconds
- [ ] Save/load state works for all MVP games

### Nice to Have
- [ ] All 26 cores ported
- [ ] Serial/IR communication functional
- [ ] Debug interface feature-complete