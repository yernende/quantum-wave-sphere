#include "support/app_options.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <string_view>

TEST_CASE("application options default to an interactive run") {
    const qws::AppOptions options = qws::parse_app_options({});

    CHECK(options.run_mode == qws::RunMode::interactive);
    CHECK_FALSE(options.show_help);
    CHECK_FALSE(options.list_geometries);
    CHECK_FALSE(options.frame_capture.has_value());
}

TEST_CASE("smoke mode is selected explicitly") {
    constexpr std::array arguments{std::string_view{"--smoke-test"}};
    const qws::AppOptions options = qws::parse_app_options(arguments);

    CHECK(options.run_mode == qws::RunMode::smoke_test);
}

TEST_CASE("help aliases are accepted") {
    constexpr std::array aliases{std::string_view{"--help"}, std::string_view{"-h"}};

    for (const std::string_view alias : aliases) {
        const std::array arguments{alias};
        const qws::AppOptions options = qws::parse_app_options(arguments);

        CAPTURE(alias);
        CHECK(options.show_help);
    }
}

TEST_CASE("geometry listing is selected explicitly") {
    constexpr std::array arguments{std::string_view{"--list-geometries"}};
    const qws::AppOptions options = qws::parse_app_options(arguments);

    CHECK(options.list_geometries);
    CHECK(options.run_mode == qws::RunMode::interactive);
}

TEST_CASE("raw capture options are parsed together") {
    constexpr std::array arguments{
        std::string_view{"--capture-raw"}, std::string_view{"frames.rgb"},
        std::string_view{"--geometry"},    std::string_view{"torus"},
        std::string_view{"--fps"},         std::string_view{"12"},
        std::string_view{"--width"},       std::string_view{"320"},
        std::string_view{"--height"},      std::string_view{"180"},
    };
    const qws::AppOptions options = qws::parse_app_options(arguments);

    REQUIRE(options.frame_capture.has_value());
    const qws::FrameCaptureOptions capture_options =
        options.frame_capture.value_or(qws::FrameCaptureOptions{});
    CHECK(options.run_mode == qws::RunMode::interactive);
    CHECK(capture_options.output_file == "frames.rgb");
    CHECK(capture_options.geometry_slug == "torus");
    CHECK(capture_options.frames_per_second == 12);
    CHECK(capture_options.width == 320);
    CHECK(capture_options.height == 180);
}

TEST_CASE("raw capture requires a geometry") {
    constexpr std::array arguments{std::string_view{"--capture-raw"},
                                   std::string_view{"frames.rgb"}};

    try {
        static_cast<void>(qws::parse_app_options(arguments));
        FAIL("Expected an invalid_argument exception");
    } catch (const std::invalid_argument& error) {
        CHECK(std::string_view{error.what()} == "--capture-raw requires --geometry SLUG.");
    }
}

TEST_CASE("unknown options fail with a useful diagnostic") {
    constexpr std::array arguments{std::string_view{"--sphere-now"}};

    try {
        static_cast<void>(qws::parse_app_options(arguments));
        FAIL("Expected an invalid_argument exception");
    } catch (const std::invalid_argument& error) {
        CHECK(std::string_view{error.what()} == "Unknown argument: --sphere-now");
    }
}
