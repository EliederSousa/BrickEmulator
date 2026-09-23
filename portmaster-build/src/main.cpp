#include "brickemu/Config.hpp"
#include "brickemu/EmulatorRuntime.hpp"
#ifdef HAVE_SDL2
#include "brickemu/Frontend_SDL.hpp"
#endif
#ifdef HAVE_TERM
#include "brickemu/Frontend_Term.hpp"
#endif

#include <filesystem>
#include <iostream>
#include <charconv>
#include <string_view>

namespace {

void printUsage(std::string_view program) {
    std::cerr << "Usage: " << program << " --brick <path-to-brick-config> [--steps N] [--trace] [--run] [--term] [--shot N] [--script a,b,..]\n"
              << "  --run: SDL2 real-time frontend (only in brickemu-sdl builds)\n"
              << "  --term: terminal frontend, interactive (only in brickemu-term builds)\n"
              << "  --pf: playfield mode: 10x17 matrix as single 'x'/' ' (with --term/--shot/--script)\n"
              << "  --no-border: skip the ╔═╗║╚═╝ outline (machine-readable dumps)\n"
              << "  --hold-ms N: key release delay after last repeat (default 200)\n"
              << "  --shot N: dump text frame after ~N frames and exit (with --script: run it first)\n"
              << "  --script: comma list on,start,left,right,down,rotate,mute,reset\n"
              << "  --cell N: square NxN LCD pixels per text cell (exact mode;\n"
              << "            default geometry is 16x8 to match terminal aspect)\n"
              << "  --cell-w N / --cell-h N: cell width/height override\n";
}

} // namespace

int main(int argc, char** argv) {
    std::filesystem::path brickPath = "../assets/E23PlusMarkII96in1.brick";
    std::uint32_t steps = 0;
    bool trace = false;
    bool run = false;
    bool term = false;
    bool shot = false;
    int shotFrame = -1;
    int cellW = 16;
    int cellH = 8;
    bool playfield = false;
    int holdMs = 200;
    bool noBorder = false;
    std::string script;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if ((arg == "--brick" || arg == "-brick") && i + 1 < argc) {
            brickPath = argv[++i];
        } else if (arg == "--steps" && i + 1 < argc) {
            const std::string_view value(argv[++i]);
            const auto* begin = value.data();
            const auto* end = begin + value.size();
            if (std::from_chars(begin, end, steps).ec != std::errc{}) {
                std::cerr << "Invalid --steps value\n";
                return 2;
            }
        } else if (arg == "--trace") {
            trace = true;
        } else if (arg == "--run") {
            run = true;
        } else if (arg == "--term") {
            term = true;
        } else if (arg == "--shot" && i + 1 < argc) {
            const std::string_view value(argv[++i]);
            const auto* begin = value.data();
            const auto* end = begin + value.size();
            if (std::from_chars(begin, end, shotFrame).ec != std::errc{}) {
                std::cerr << "Invalid --shot value\n";
                return 2;
            }
            shot = true;
        } else if (arg == "--script" && i + 1 < argc) {
            script = argv[++i];
        } else if (arg == "--pf") {
            playfield = true;
        } else if (arg == "--no-border") {
            noBorder = true;
        } else if (arg == "--hold-ms" && i + 1 < argc) {
            const std::string_view value(argv[++i]);
            const auto* begin = value.data();
            const auto* end = begin + value.size();
            if (std::from_chars(begin, end, holdMs).ec != std::errc{} ||
                holdMs <= 0) {
                std::cerr << "Invalid --hold-ms value\n";
                return 2;
            }
        } else if (arg == "--cell" && i + 1 < argc) {
            int cell = 0;
            const std::string_view value(argv[++i]);
            const auto* begin = value.data();
            const auto* end = begin + value.size();
            if (std::from_chars(begin, end, cell).ec != std::errc{} ||
                cell <= 0) {
                std::cerr << "Invalid --cell value\n";
                return 2;
            }
            cellW = cellH = cell; // square LCD blocks (exact mode)
        } else if (arg == "--cell-w" && i + 1 < argc) {
            const std::string_view value(argv[++i]);
            const auto* begin = value.data();
            const auto* end = begin + value.size();
            if (std::from_chars(begin, end, cellW).ec != std::errc{} ||
                cellW <= 0) {
                std::cerr << "Invalid --cell-w value\n";
                return 2;
            }
        } else if (arg == "--cell-h" && i + 1 < argc) {
            const std::string_view value(argv[++i]);
            const auto* begin = value.data();
            const auto* end = begin + value.size();
            if (std::from_chars(begin, end, cellH).ec != std::errc{} ||
                cellH <= 0) {
                std::cerr << "Invalid --cell-h value\n";
                return 2;
            }
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << '\n';
            printUsage(argv[0]);
            return 2;
        }
    }

    brickemu::ConfigError error;
    auto config = brickemu::ConfigLoader::load(brickPath, &error);
    if (!config) {
        std::cerr << error.message << '\n';
        return 1;
    }

#ifdef HAVE_SDL2
    if (run) {
        return brickemu::runSdl(*config);
    }
#else
    if (run) {
        std::cerr << "--run needs an SDL2 build (brickemu-sdl)\n";
        return 2;
    }
#endif
#ifdef HAVE_TERM
    if (term || shot || !script.empty() || playfield) {
        brickemu::TermOptions options;
        options.interactive = (term || playfield) && !shot && script.empty();
        options.cellW = cellW;
        options.cellH = cellH;
        options.playfield = playfield;
        options.holdMs = holdMs;
        options.noBorder = noBorder;
        if (!term && !shot && !script.empty()) {
            options.interactive = false; // --script alone: run it, dump, exit
        }
        options.shotFrame = shotFrame;
        options.script = script;
        return brickemu::runTerm(*config, options);
    }
#else
    if (term || shot || !script.empty() || playfield) {
        std::cerr << "--term/--shot/--script/--pf need a terminal build (brickemu-term)\n";
        return 2;
    }
#endif

    brickemu::EmulatorRuntime runtime(std::move(*config));
    if (!runtime.initialize()) {
        return 3;
    }
    runtime.runSteps(steps, trace);
    if (steps > 0) {
        const auto& cpu = runtime.config();
        std::cout << "C++ step " << steps << ": done\n";
    }
    return 0;
}
