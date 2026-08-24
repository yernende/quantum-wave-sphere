#pragma once

#include "capture/animation_capture.hpp"

#include <charconv>
#include <cstddef>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace qws {

enum class RunMode {
    interactive,
    smoke_test,
};

struct AppOptions {
    RunMode run_mode{RunMode::interactive};
    bool show_help{false};
    bool list_geometries{false};
    std::optional<FrameCaptureOptions> frame_capture;
};

[[nodiscard]] inline std::string_view usage() noexcept {
    return "Usage: quantum-wave-sphere [--smoke-test] [--list-geometries] [--help]\n"
           "       quantum-wave-sphere --capture-raw FILE --geometry SLUG [options]\n"
           "\n"
           "  --smoke-test       Render three frames and exit.\n"
           "  --list-geometries  Print every capture-compatible geometry slug.\n"
           "  --capture-raw FILE Write one exact animation period as headerless RGB24 frames.\n"
           "  --geometry SLUG    Select the geometry for raw capture.\n"
           "  --fps NUMBER       Capture sampling rate (default: 24).\n"
           "  --width NUMBER     Capture width in pixels (default: 640).\n"
           "  --height NUMBER    Capture height in pixels (default: 360).\n";
}

[[nodiscard]] inline AppOptions parse_app_options(std::span<const std::string_view> arguments) {
    AppOptions options{};
    FrameCaptureOptions capture_options{};
    bool has_capture_output = false;
    bool has_capture_geometry = false;
    bool has_capture_only_option = false;

    const auto require_value = [&arguments](std::size_t& index,
                                            std::string_view option) -> std::string_view {
        if (index + 1 >= arguments.size()) {
            throw std::invalid_argument{"Missing value for " + std::string{option} + '.'};
        }

        ++index;
        return arguments[index];
    };

    const auto parse_positive_integer = [](std::string_view value, std::string_view option) {
        int result = 0;
        const auto [end, error] =
            std::from_chars(value.data(), value.data() + value.size(), result);
        if (error != std::errc{} || end != value.data() + value.size() || result <= 0) {
            throw std::invalid_argument{std::string{option} + " expects a positive integer."};
        }

        return result;
    };

    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const std::string_view argument = arguments[index];
        if (argument == "--smoke-test") {
            options.run_mode = RunMode::smoke_test;
        } else if (argument == "--list-geometries") {
            options.list_geometries = true;
        } else if (argument == "--capture-raw") {
            capture_options.output_file = require_value(index, argument);
            has_capture_output = true;
        } else if (argument == "--geometry") {
            capture_options.geometry_slug = require_value(index, argument);
            has_capture_geometry = true;
            has_capture_only_option = true;
        } else if (argument == "--fps") {
            capture_options.frames_per_second =
                parse_positive_integer(require_value(index, argument), argument);
            has_capture_only_option = true;
        } else if (argument == "--width") {
            capture_options.width =
                parse_positive_integer(require_value(index, argument), argument);
            has_capture_only_option = true;
        } else if (argument == "--height") {
            capture_options.height =
                parse_positive_integer(require_value(index, argument), argument);
            has_capture_only_option = true;
        } else if (argument == "--help" || argument == "-h") {
            options.show_help = true;
        } else {
            throw std::invalid_argument{"Unknown argument: " + std::string{argument}};
        }
    }

    if (has_capture_output) {
        if (options.run_mode == RunMode::smoke_test || options.list_geometries) {
            throw std::invalid_argument{
                "--capture-raw cannot be combined with --smoke-test or --list-geometries."};
        }
        if (!has_capture_geometry || capture_options.geometry_slug.empty()) {
            throw std::invalid_argument{"--capture-raw requires --geometry SLUG."};
        }
        if (capture_options.output_file.empty()) {
            throw std::invalid_argument{"--capture-raw requires a non-empty output file."};
        }

        options.frame_capture = std::move(capture_options);
    } else if (has_capture_only_option) {
        throw std::invalid_argument{
            "--geometry, --fps, --width, and --height require --capture-raw."};
    }

    if (options.list_geometries && options.run_mode == RunMode::smoke_test) {
        throw std::invalid_argument{"--list-geometries cannot be combined with --smoke-test."};
    }

    return options;
}

[[nodiscard]] inline AppOptions parse_app_options(int argc, char* const argv[]) {
    std::vector<std::string_view> arguments{};
    if (argc > 1) {
        arguments.reserve(static_cast<std::size_t>(argc - 1));
    }

    for (int index = 1; index < argc; ++index) {
        arguments.emplace_back(argv[index]);
    }

    return parse_app_options(arguments);
}

} // namespace qws
