#!/usr/bin/env bash
# run_tests.sh - build + run both SPL03 instruction test suites and compare.
set -u
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(dirname "$HERE")"
BIN="/tmp/opencode/test_instructions_cpp"

echo "=== 1/3 Python suite (reference vs C++ mirror) ==="
python3 "$HERE/test_instructions.py" "$@" | tee /tmp/opencode/py_tests.log
PY_RC=${PIPESTATUS[0]}
echo "(python exit: $PY_RC -- 0 means all reference vectors pass)"

echo
echo "=== 2/3 Build C++ suite ==="
mkdir -p /tmp/opencode
g++ -std=c++20 -Wall -Wextra -Wpedantic "$HERE/test_instructions.cpp" -o "$BIN"
echo "(build exit: $?)"

echo
echo "=== 3/3 C++ suite (port logic vs Python parity) ==="
"$BIN" "$@" | tee /tmp/opencode/cpp_tests.log
CPP_RC=${PIPESTATUS[0]}
echo "(c++ exit: $CPP_RC -- 0 means harness self-checks pass)"

echo
echo "=== 4/4 DisplayBuffer unit tests (SDL-free) ==="
g++ -std=c++20 -Wall -Wextra -Wpedantic -I"$ROOT/portmaster-build/include" \
  "$HERE/test_display.cpp" -o /tmp/opencode/test_display
/tmp/opencode/test_display | tee /tmp/opencode/display_tests.log
DISP_RC=${PIPESTATUS[0]}
echo "(display exit: $DISP_RC)"

echo
echo "=== 5/5 HeldKeys input unit tests ==="
g++ -std=c++20 -Wall -Wextra -Wpedantic -I"$ROOT/portmaster-build/include" \
  "$HERE/test_term_input.cpp" -o /tmp/opencode/test_term_input
/tmp/opencode/test_term_input | tee /tmp/opencode/input_tests.log
INPUT_RC=${PIPESTATUS[0]}
echo "(input exit: $INPUT_RC)"

echo
echo "=== 6/6 HUD decoder unit tests ==="
g++ -std=c++20 -Wall -Wextra -Wpedantic -I"$ROOT/portmaster-build/include" \
  "$HERE/test_hud.cpp" -o /tmp/opencode/test_hud
/tmp/opencode/test_hud | tee /tmp/opencode/hud_tests.log
HUD_RC=${PIPESTATUS[0]}
echo "(hud exit: $HUD_RC)"

echo
echo "=== Summary ==="
grep -E "^(Vectors checked|Passed|Failed|PORT MISMATCHES|Parity|Harness)" /tmp/opencode/py_tests.log /tmp/opencode/cpp_tests.log || true
echo
echo "Parity-gap counts (must agree between languages):"
echo -n "  python PORT-MISMATCH lines: "; grep -c "PORT-MISMATCH" /tmp/opencode/py_tests.log || true
echo -n "  c++    PARITY-GAP lines:    "; grep -c "PARITY-GAP" /tmp/opencode/cpp_tests.log || true
