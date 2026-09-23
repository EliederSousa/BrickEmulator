// test_term_input.cpp - unit tests for HeldKeys (terminal held-key tracker).
// Build: g++ -std=c++20 -Wall -Wextra -Wpedantic -Iportmaster-build/include
//          testbed/test_term_input.cpp -o /tmp/opencode/test_term_input
#include "brickemu/Frontend_Term.hpp"

#include <cstdio>

namespace {
int fails = 0;
void expect(bool cond, const char* what) {
    if (!cond) {
        ++fails;
        std::printf("  FAIL %s\n", what);
    } else {
        std::printf("  PASS %s\n", what);
    }
}
const brickemu::ButtonMap kLeft{"left", "PA", 8};
const brickemu::ButtonMap kRight{"right", "PA", 16};
using brickemu::HeldKeys;
HeldKeys::Clock::time_point at(int ms) {
    return HeldKeys::Clock::time_point(std::chrono::milliseconds(ms));
}
} // namespace

int main() {
    using namespace std::chrono_literals;
    // 1. single press: held until deadline, then expires exactly once
    {
        HeldKeys h;
        h.press(&kLeft, at(0), 200ms);
        expect(h.size() == 1, "press adds entry");
        expect(h.expired(at(199)).empty(), "no expiry before deadline");
        auto out = h.expired(at(200));
        expect(out.size() == 1 && out[0] == &kLeft, "expires at deadline");
        expect(h.expired(at(1000)).empty(), "no double expiry");
    }
    // 2. auto-repeat refreshes: no gap while repeats keep coming
    {
        HeldKeys h;
        h.press(&kRight, at(0), 200ms);
        for (int t = 30; t < 1000; t += 30) {
            h.press(&kRight, at(t), 200ms); // terminal key repeat
        }
        expect(h.size() == 1, "repeats never stack");
        expect(h.expired(at(999)).empty(), "held continuously, no gaps");
        auto out = h.expired(at(999 + 200));
        expect(out.size() == 1 && out[0] == &kRight,
               "releases 200ms after last repeat");
    }
    // 3. two buttons are independent
    {
        HeldKeys h;
        h.press(&kLeft, at(0), 200ms);
        h.press(&kRight, at(50), 200ms);
        expect(h.size() == 2, "two buttons tracked");
        auto out = h.expired(at(200));
        expect(out.size() == 1 && out[0] == &kLeft, "left expires first");
        out = h.expired(at(250));
        expect(out.size() == 1 && out[0] == &kRight, "right expires later");
    }
    // 4. re-press after expiry starts a fresh hold
    {
        HeldKeys h;
        h.press(&kLeft, at(0), 200ms);
        h.expired(at(500));
        h.press(&kLeft, at(600), 200ms);
        expect(h.expired(at(799)).empty(), "fresh hold respected");
        expect(h.expired(at(800)).size() == 1, "fresh hold expires");
    }
    std::printf(fails == 0 ? "ALL INPUT TESTS PASS\n" : "FAILURES: %d\n", fails);
    return fails == 0 ? 0 : 1;
}
