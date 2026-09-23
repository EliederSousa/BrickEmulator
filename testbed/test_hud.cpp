// test_hud.cpp - unit tests for the Apollo 18 HUD decoder (SDL-free).
// Build: g++ -std=c++20 -Wall -Wextra -Wpedantic -Iportmaster-build/include
//          testbed/test_hud.cpp -o /tmp/opencode/test_hud
#include "brickemu/Apollo18Hud.hpp"

#include <cstdio>
#include <cstring>

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
void setBit(std::uint8_t* ram, brickemu::Apollo18SegBit s) {
    ram[s.byte] |= (1U << s.bit);
}
} // namespace

int main() {
    using namespace brickemu;
    // 1. all blank
    {
        std::uint8_t ram[0x30] = {};
        Apollo18Hud h = decodeHud(ram);
        expect(h.d[0] == -1 && h.d[1] == -1 && h.speed == -1 &&
                   h.level == -1 && !h.sound && !h.runner && !h.coffee,
               "blank VRAM decodes blank");
    }
    // 2. all-on: every digit reads 8, icons on, next rows full
    {
        std::uint8_t ram[0x30];
        std::memset(ram, 0xFF, sizeof ram);
        Apollo18Hud h = decodeHud(ram);
        expect(h.d[0] == 1 && h.d[1] == 8 && h.d[2] == 8 && h.d[3] == 8 &&
                   h.d[4] == 8 && h.speed == 8 && h.level == 8,
               "all-on digits read 8 (lead 1)");
        expect(h.sound && h.dot && h.runner && h.alarm && h.coffee,
               "all-on icons on");
        expect(h.next[0] == 0xF && h.next[3] == 0xF, "all-on next full");
    }
    // 3. each digit 0-9 decodes (drive D1 through all patterns)
    {
        bool ok = true;
        const int want[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        const std::uint8_t masks[10] = {119, 36, 93, 109, 46,
                                        107, 123, 37, 127, 111};
        for (int d = 0; d < 10; ++d) {
            std::uint8_t ram[0x30] = {};
            for (int i = 0; i < 7; ++i) {
                if ((masks[d] >> i) & 1) {
                    setBit(ram, kApollo18Score[7 + i]);
                }
            }
            Apollo18Hud h = decodeHud(ram);
            ok &= (h.d[2] == want[d]);
        }
        expect(ok, "digits 0-9 decode on score D2");
    }
    // 4. score assembly: lead + D1..D4 -> 19888
    {
        std::uint8_t ram[0x30] = {};
        setBit(ram, kApollo18Lead1);
        auto light = [&](int di, std::uint8_t mask) {
            for (int i = 0; i < 7; ++i) {
                if ((mask >> i) & 1) {
                    setBit(ram, kApollo18Score[di * 7 + i]);
                }
            }
        };
        light(0, 111); // 9
        light(1, 127); // 8
        light(2, 127); // 8
        light(3, 127); // 8
        Apollo18Hud h = decodeHud(ram);
        expect(h.d[0] == 1 && h.d[1] == 9 && h.d[2] == 8 && h.d[3] == 8 &&
                   h.d[4] == 8,
               "score reads 19888");
    }
    // 5. speed/level single digits + next-piece pattern
    {
        std::uint8_t ram[0x30] = {};
        for (int i = 0; i < 7; ++i) {
            if ((93 >> i) & 1) {
                setBit(ram, kApollo18Speed[i]); // 2
            }
            if ((46 >> i) & 1) {
                setBit(ram, kApollo18Level[i]); // 4
            }
        }
        // next row 0: cols 0,2 lit -> bits 3,1 -> 0b1010
        setBit(ram, kApollo18Next[0]);
        setBit(ram, kApollo18Next[2]);
        Apollo18Hud h = decodeHud(ram);
        expect(h.speed == 2 && h.level == 4, "speed 2 level 4");
        expect(h.next[0] == 0xA && h.next[1] == 0x0, "next row pattern");
    }
    std::printf(fails == 0 ? "ALL HUD TESTS PASS\n" : "FAILURES: %d\n", fails);
    return fails == 0 ? 0 : 1;
}
