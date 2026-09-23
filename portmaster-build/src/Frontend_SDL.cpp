// SDL2 frontend: real-time loop for handhelds (RG35XX H class, 1 GB RAM).
//
// Efficiency choices (this whole port targets weak ARM SoCs):
// - Display-only LCD canvas (332x480) in an INDEX8 streaming texture with a
//   2-entry LCD palette: per-frame cost is one ~155 KB upload, and only when
//   the 48-byte LCDRAM actually changed (memcmp inside Apollo18Display).
// - SDL_Renderer default (accelerated where the firmware provides GLES,
//   software fallback otherwise); SDL_RenderSetLogicalSize letterboxes the
//   portrait LCD onto the 640x480 landscape screen, no manual scaling.
// - Single thread, static buffers, no per-frame allocation.
// - Software audio mixing comes in the next slice (SPL0Xsound port).

#include "brickemu/Frontend_SDL.hpp"

#include "brickemu/CoreRegistry.hpp"
#include "brickemu/DisplayBuffer.hpp"

#include <SDL.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

namespace brickemu {
namespace {

// Display-only LCD canvas (332x480) in an RGB565 streaming texture.
// (INDEX8 textures have no public palette API in SDL2, so the 1-byte index
// buffer from Apollo18Display is expanded to RGB565 on upload — still only
// on frames where the 48-byte LCDRAM changed.)
constexpr std::uint16_t kLcdBg =
    ((156 >> 3) << 11) | ((166 >> 2) << 5) | (108 >> 3);
constexpr std::uint16_t kLcdFg =
    ((30 >> 3) << 11) | ((34 >> 2) << 5) | (26 >> 3);

constexpr int kWindowW = 640;
constexpr int kWindowH = 480;

// Apollo 18 in 1 button map (from assets/Apollo18in1B0302.brick
// direct_input): button name -> (port, mask). Press drives level 1,
// release clears; RES resets the core on press.
struct ButtonMap {
    const char* name;
    const char* port;
    std::uint8_t mask;
};
constexpr ButtonMap kButtons[] = {
    {"btnOnOff", "PA", 1},  {"btnMute", "PA", 32}, {"btnStartP", "PA", 64},
    {"btnReset", "RES", 0}, {"btnLeft", "PA", 8},  {"btnRight", "PA", 16},
    {"btnDown", "PA", 4},   {"btnRotate", "PA", 2},
};

const ButtonMap* findButton(const char* name) {
    for (const auto& b : kButtons) {
        if (std::string(b.name) == name) {
            return &b;
        }
    }
    return nullptr;
}

void driveButton(ICpuCore& cpu, const ButtonMap& b, bool pressed) {
    if (std::string(b.port) == "RES") {
        if (pressed) {
            cpu.setPortInput("RES", 0, 0);
        }
        return;
    }
    cpu.setPortInput(b.port, b.mask, pressed ? 1 : -1);
}

// Keyboard fallback (also what gptokeyb targets on device).
const ButtonMap* keyButton(SDL_Keycode key) {
    switch (key) {
    case SDLK_LEFT:
    case SDLK_a: return findButton("btnLeft");
    case SDLK_RIGHT:
    case SDLK_d: return findButton("btnRight");
    case SDLK_DOWN:
    case SDLK_s: return findButton("btnDown");
    case SDLK_UP:
    case SDLK_w:
    case SDLK_SPACE: return findButton("btnRotate");
    case SDLK_RETURN: return findButton("btnStartP");
    case SDLK_o: return findButton("btnOnOff");
    case SDLK_m: return findButton("btnMute");
    case SDLK_r: return findButton("btnReset");
    default: return nullptr;
    }
}

// Gamecontroller mapping for the brick layout.
const ButtonMap* padButton(Uint8 button) {
    switch (button) {
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return findButton("btnLeft");
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return findButton("btnRight");
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return findButton("btnDown");
    case SDL_CONTROLLER_BUTTON_A: return findButton("btnRotate");
    case SDL_CONTROLLER_BUTTON_START: return findButton("btnStartP");
    case SDL_CONTROLLER_BUTTON_BACK: return findButton("btnMute");
    case SDL_CONTROLLER_BUTTON_Y: return findButton("btnOnOff");
    case SDL_CONTROLLER_BUTTON_X: return findButton("btnReset");
    default: return nullptr;
    }
}

} // namespace

int runSdl(const BrickConfig& config) {
    std::string error;
    auto cpu = CoreRegistry::create(config, &error);
    if (!cpu) {
        std::fprintf(stderr, "failed to create core: %s\n", error.c_str());
        return 3;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 4;
    }

    SDL_Window* window = SDL_CreateWindow(
        "BrickEmu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        kWindowW, kWindowH, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 4;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) { // software fallback (KMSDRM without GL, etc.)
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 4;
    }
    SDL_RenderSetLogicalSize(renderer, Apollo18Display::kWidth,
                             Apollo18Display::kHeight);

    SDL_Texture* lcd = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
        Apollo18Display::kWidth, Apollo18Display::kHeight);
    if (!lcd) {
        std::fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 4;
    }
    // Static upload buffer (332*480*2 B ~ 311 KB, .bss, no per-frame alloc).
    static std::uint16_t s_upload[Apollo18Display::kPixels];

    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            SDL_GameControllerOpen(i);
            break; // first pad is enough for a brick handheld
        }
    }

    Apollo18Display display;
    const std::uint32_t cyclesPerSec =
        config.clockHz != 0 ? config.clockHz : 690000U;
    const double cyclesPerFrame = (double)cyclesPerSec / 60.0;
    // Persistent debt: total emulated cycles track the ideal 60 Hz timeline
    // within one instruction (no per-frame drift).
    double cycleDebt = 0.0;
    auto nextDeadline = std::chrono::steady_clock::now();

    const char* dumpPath = std::getenv("BRICKEMU_DUMPFRAME");
    const int dumpAt =
        std::getenv("BRICKEMU_DUMPFRAME_AT") ? std::atoi(std::getenv("BRICKEMU_DUMPFRAME_AT")) : 600;
    int frames = 0;
    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT: running = false; break;
            case SDL_KEYDOWN:
                if (ev.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                } else if (const ButtonMap* b = keyButton(ev.key.keysym.sym)) {
                    driveButton(*cpu, *b, true);
                }
                break;
            case SDL_KEYUP:
                if (const ButtonMap* b = keyButton(ev.key.keysym.sym)) {
                    driveButton(*cpu, *b, false);
                }
                break;
            case SDL_CONTROLLERBUTTONDOWN:
                if (const ButtonMap* b = padButton(ev.cbutton.button)) {
                    driveButton(*cpu, *b, true);
                }
                break;
            case SDL_CONTROLLERBUTTONUP:
                if (const ButtonMap* b = padButton(ev.cbutton.button)) {
                    driveButton(*cpu, *b, false);
                }
                break;
            default: break;
            }
        }
        if (!running) {
            break;
        }

        // Fixed timestep: exactly one frame's worth of cycles per frame
        // (deterministic, constant speed on device; vsync paces present).
        cycleDebt += cyclesPerFrame;
        while (cycleDebt >= 1.0) {
            cycleDebt -= (double)cpu->clock();
        }

        if (display.render(cpu->vram().data())) {
            const auto& px = display.pixels();
            for (std::uint32_t i = 0; i < Apollo18Display::kPixels; ++i) {
                s_upload[i] = px[i] ? kLcdFg : kLcdBg;
            }
            SDL_Rect full{0, 0, Apollo18Display::kWidth,
                          Apollo18Display::kHeight};
            SDL_UpdateTexture(lcd, &full, s_upload,
                              Apollo18Display::kWidth * 2);
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, lcd, nullptr, nullptr);
        SDL_RenderPresent(renderer);
        ++frames;

        // Without vsync (software fallback), pace to 60 Hz manually.
        nextDeadline += std::chrono::microseconds(1000000 / 60);
        Uint32 remaining = 0;
        {
            const auto now = std::chrono::steady_clock::now();
            if (nextDeadline > now) {
                remaining = (Uint32)std::chrono::duration_cast<std::chrono::milliseconds>(
                    nextDeadline - now).count();
            } else if (now > nextDeadline + std::chrono::milliseconds(250)) {
                nextDeadline = now;
            }
        }
        if (remaining > 0) {
            SDL_Delay(remaining);
        }

        if (dumpPath && frames >= dumpAt) {
            SDL_Surface* shot = SDL_CreateRGBSurfaceWithFormat(
                0, Apollo18Display::kWidth, Apollo18Display::kHeight, 24,
                SDL_PIXELFORMAT_RGB24);
            if (shot) {
                SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGB24,
                                     shot->pixels, shot->pitch);
                SDL_SaveBMP(shot, dumpPath);
                SDL_FreeSurface(shot);
            }
            running = false;
        }
    }

    SDL_DestroyTexture(lcd);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

} // namespace brickemu
