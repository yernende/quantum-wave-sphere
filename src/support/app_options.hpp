#pragma once

#include <cstddef>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace qws {

enum class RunMode {
    interactive,
    smoke_test,
};

struct AppOptions {
    RunMode run_mode{RunMode::interactive};
    bool show_help{false};
};

[[nodiscard]] inline std::string_view usage() noexcept {
    return "Usage: quantum-wave-sphere [--smoke-test] [--help]\n"
           "  --smoke-test  Render three frames and exit.\n";
}

[[nodiscard]] inline AppOptions parse_app_options(std::span<const std::string_view> arguments) {
    AppOptions options{};

    for (const std::string_view argument : arguments) {
        if (argument == "--smoke-test") {
            options.run_mode = RunMode::smoke_test;
        } else if (argument == "--help" || argument == "-h") {
            options.show_help = true;
        } else {
            throw std::invalid_argument{"Unknown argument: " + std::string{argument}};
        }
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
