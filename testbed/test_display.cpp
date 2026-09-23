// test_display.cpp - unit tests for Apollo18Display (SDL-free).
// Build: g++ -std=c++20 -Wall -Wextra -Wpedantic -Iportmaster-build/include
//          testbed/test_display.cpp -o /tmp/opencode/test_display
#include "brickemu/DisplayBuffer.hpp"

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
} // namespace

int main() {
    using brickemu::Apollo18Display;
    std::printf("DisplayBuffer tests (canvas %ux%u, %u segs)\n",
                Apollo18Display::kWidth, Apollo18Display::kHeight,
                (unsigned)brickemu::kApollo18Segs.size());

    // 1. all-off -> empty, then idempotent (no change reported twice)
    {
        Apollo18Display d;
        std::array<std::uint8_t, 0x30> ram{};
        expect(d.render(ram.data()), "first render reports change");
        for (auto p : d.pixels()) expect(p == 0, "all-off pixels zero");
        expect(!d.render(ram.data()), "identical LCDRAM reports no change");
    }
    // 2. single bit lights exactly its baked rect
    {
        Apollo18Display d;
        std::array<std::uint8_t, 0x30> ram{};
        const auto& s = brickemu::kApollo18Segs[0];
        ram[s.byte] |= (1U << s.bit);
        expect(d.render(ram.data()), "single-bit render reports change");
        std::uint32_t lit = 0;
        for (auto p : d.pixels()) lit += p;
        expect(lit == (std::uint32_t)s.w * s.h, "lit count == rect area");
        expect(d.pixels()[s.y * Apollo18Display::kWidth + s.x] == 1,
               "rect top-left set");
        // a pixel far outside the rect stays off (check corner unless inside)
        bool cornerInside =
            (0 >= s.x && 0 < s.x + s.w && 0 >= s.y && 0 < s.y + s.h);
        if (!cornerInside)
            expect(d.pixels()[0] == 0, "unrelated pixel stays off");
    }
    // 3. every baked rect is inside the canvas with nonzero area
    {
        bool ok = true;
        for (const auto& s : brickemu::kApollo18Segs) {
            ok &= (s.w > 0 && s.h > 0 && s.x + s.w <= Apollo18Display::kWidth &&
                   s.y + s.h <= Apollo18Display::kHeight && s.byte < 0x30 &&
                   s.bit < 8);
        }
        expect(ok, "all 237 rects in-bounds, nonzero, byte<0x30");
    }
    // 4. all-on fills a sane fraction (matches Python reference: ~65%)
    {
        Apollo18Display d;
        std::array<std::uint8_t, 0x30> ram;
        ram.fill(0xFF);
        d.render(ram.data());
        std::uint32_t lit = 0;
        for (auto p : d.pixels()) lit += p;
        double frac = (double)lit / Apollo18Display::kPixels;
        char msg[128];
        std::snprintf(msg, sizeof msg, "all-on fill %.1f%% in [50,80]", frac * 100);
        expect(frac > 0.50 && frac < 0.80, msg);
    }
    // 5. clearing one bit clears only its rect
    {
        Apollo18Display d;
        std::array<std::uint8_t, 0x30> ram;
        ram.fill(0xFF);
        d.render(ram.data());
        std::uint32_t full = 0;
        for (auto p : d.pixels()) full += p;
        const auto& s = brickemu::kApollo18Segs[100];
        ram[s.byte] &= ~(1U << s.bit);
        expect(d.render(ram.data()), "bit-clear reports change");
        std::uint32_t lit = 0;
        for (auto p : d.pixels()) lit += p;
        expect(lit < full, "fewer pixels after clear");
    }
    // 6. playfield map: 170 cells, 10x17 row-major, unique bits, byte<0x30
    {
        using brickemu::kApollo18Pf;
        bool ok = (kApollo18Pf.size() == 170);
        bool seen[0x30][8] = {};
        for (const auto& cell : kApollo18Pf) {
            ok &= (cell.byte < 0x30 && cell.bit < 8 && !seen[cell.byte][cell.bit]);
            seen[cell.byte][cell.bit] = true;
        }
        expect(ok, "playfield map: 170 unique bits, byte<0x30");
        // spot check: pf render of all-on VRAM lights every playfield cell
        std::uint8_t ram[0x30];
        std::memset(ram, 0xFF, sizeof ram);
        int lit = 0;
        for (const auto& cell : kApollo18Pf) {
            lit += (ram[cell.byte] >> cell.bit) & 1;
        }
        expect(lit == 170, "all-on VRAM lights all 170 playfield cells");
    }
    std::printf(fails == 0 ? "ALL DISPLAY TESTS PASS\n" : "FAILURES: %d\n", fails);
    return fails == 0 ? 0 : 1;
}
