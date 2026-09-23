#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace brickemu {

struct ButtonConfig {
    std::string name;
    std::vector<int> hotKeys;
};

struct DisplayConfig {
    double motionBlur = 0.6;
    double ghostSegments = 0.0;
    double shadow = 5.0;
};

struct MaskOptions {
    std::unordered_map<std::string, int> portPullup;
    std::unordered_map<std::string, int> portPkey;
    int nonCrystalDiv = 0;
};

struct BrickConfig {
    std::string id;
    std::string core;
    std::filesystem::path configPath;
    std::filesystem::path facePath;
    std::filesystem::path romPath;
    std::filesystem::path soundRomPath;
    std::uint32_t clockHz = 0;
    DisplayConfig display;
    MaskOptions maskOptions;
    std::vector<ButtonConfig> buttons;
};

struct ConfigError {
    std::string message;
};

class ConfigLoader {
public:
    static std::optional<BrickConfig> load(const std::filesystem::path& path, ConfigError* error = nullptr);

private:
    static std::filesystem::path resolveRelative(const std::filesystem::path& configPath, const std::string& value);
};

} // namespace brickemu
