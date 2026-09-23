#pragma once

// SDL2 frontend entry point (built only when SDL2 is available).
// Owns the real-time loop: pace the CPU at config.clockHz, push VRAM into
// an INDEX8 LCD texture at 60 Hz (only when LCDRAM changed), and map
// keyboard + gamecontroller input onto the core's PA-port buttons.

#include "brickemu/Config.hpp"

namespace brickemu {

// Runs until SDL_QUIT/Escape, or until BRICKEMU_DUMPFRAME_AT frames have
// been presented when BRICKEMU_DUMPFRAME is set (headless frame check).
// Returns 0 on success.
int runSdl(const BrickConfig& config);

} // namespace brickemu
