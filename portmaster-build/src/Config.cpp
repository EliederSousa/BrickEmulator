#include "brickemu/Config.hpp"

#include <charconv>
#include <fstream>
#include <regex>
#include <sstream>
#include <utility>

namespace brickemu {
namespace {

std::optional<std::string> extractString(const std::string& json, const std::string& key) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        return std::nullopt;
    }
    return match[1].str();
}

std::optional<std::uint32_t> extractUInt(const std::string& json, const std::string& key) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*([0-9]+)");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        return std::nullopt;
    }

    std::uint32_t value = 0;
    const auto text = match[1].str();
    const auto* begin = text.data();
    const auto* end = begin + text.size();
    if (std::from_chars(begin, end, value).ec != std::errc{}) {
        return std::nullopt;
    }
    return value;
}

std::optional<double> extractDouble(const std::string& json, const std::string& key) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*([-+]?[0-9]*\\.?[0-9]+)");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        return std::nullopt;
    }
    try {
        return std::stod(match[1].str());
    } catch (...) {
        return std::nullopt;
    }
}

std::unordered_map<std::string, int> extractStringIntObject(const std::string& json, const std::string& key) {
    std::unordered_map<std::string, int> values;
    const std::regex objectPattern("\\\"" + key + "\\\"\\s*:\\s*\\{([^}]*)\\}");
    std::smatch objectMatch;
    if (!std::regex_search(json, objectMatch, objectPattern)) {
        return values;
    }

    const std::string body = objectMatch[1].str();
    const std::regex entryPattern("\\\"([^\\\"]+)\\\"\\s*:\\s*(-?[0-9]+)");
    for (auto it = std::sregex_iterator(body.begin(), body.end(), entryPattern); it != std::sregex_iterator(); ++it) {
        int value = 0;
        const auto text = (*it)[2].str();
        const auto* begin = text.data();
        const auto* end = begin + text.size();
        if (std::from_chars(begin, end, value).ec == std::errc{}) {
            values.emplace((*it)[1].str(), value);
        }
    }
    return values;
}

void setError(ConfigError* error, std::string message) {
    if (error != nullptr) {
        error->message = std::move(message);
    }
}

} // namespace

std::optional<BrickConfig> ConfigLoader::load(const std::filesystem::path& path, ConfigError* error) {
    std::ifstream file(path);
    if (!file) {
        setError(error, "failed to open config: " + path.string());
        return std::nullopt;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    const auto json = buffer.str();

    BrickConfig config;
    config.configPath = path;

    const auto id = extractString(json, "id");
    const auto core = extractString(json, "core");
    const auto facePath = extractString(json, "face_path");
    const auto clock = extractUInt(json, "clock");

    if (!id || !core || !facePath || !clock) {
        setError(error, "config is missing one of: id, core, face_path, clock");
        return std::nullopt;
    }

    config.id = *id;
    config.core = *core;
    config.facePath = resolveRelative(path, *facePath);
    config.clockHz = *clock;

    if (const auto romPath = extractString(json, "rom_path")) {
        config.romPath = resolveRelative(path, *romPath);
    }
    if (const auto soundRomPath = extractString(json, "sound_rom_path")) {
        config.soundRomPath = resolveRelative(path, *soundRomPath);
    }
    if (const auto motionBlur = extractDouble(json, "motion_blur")) {
        config.display.motionBlur = *motionBlur;
    }
    if (const auto ghostSegments = extractDouble(json, "ghost_segments")) {
        config.display.ghostSegments = *ghostSegments;
    }
    if (const auto shadow = extractDouble(json, "shadow")) {
        config.display.shadow = *shadow;
    }
    if (const auto nonCrystalDiv = extractUInt(json, "non_crystal_div")) {
        config.maskOptions.nonCrystalDiv = static_cast<int>(*nonCrystalDiv);
    }
    config.maskOptions.portPullup = extractStringIntObject(json, "port_pullup");
    config.maskOptions.portPkey = extractStringIntObject(json, "port_pkey");

    return config;
}

std::filesystem::path ConfigLoader::resolveRelative(const std::filesystem::path& configPath, const std::string& value) {
    const std::filesystem::path raw(value);
    if (raw.is_absolute()) {
        return raw;
    }

    // .brick files in the Python app use paths relative to the repository root.
    const auto fromWorkingDirectory = std::filesystem::weakly_canonical(raw);
    if (std::filesystem::exists(fromWorkingDirectory)) {
        return fromWorkingDirectory;
    }

    return std::filesystem::weakly_canonical(configPath.parent_path() / raw);
}

} // namespace brickemu
