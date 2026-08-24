#include "capture/animation_capture.hpp"

#include "bootstrap/glfw_context.hpp"
#include "demo/showcase_scene.hpp"
#include "demo/wave_mesh.hpp"
#include "graphics/geometry_catalog.hpp"
#include "support/opengl_diagnostics.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <glad/gl.h>
#include <iostream>
#include <limits>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace qws {
namespace {

constexpr double maximum_capture_duration_seconds = 30.0;

struct CaptureGeometry {
    GeometryKind kind;
    std::string_view slug;
};

inline constexpr std::array capture_geometries{
    CaptureGeometry{.kind = GeometryKind::icosphere, .slug = "icosphere"},
    CaptureGeometry{.kind = GeometryKind::torus, .slug = "torus"},
    CaptureGeometry{.kind = GeometryKind::superellipsoid, .slug = "superellipsoid"},
    CaptureGeometry{.kind = GeometryKind::trefoil_knot, .slug = "trefoil-knot"},
    CaptureGeometry{.kind = GeometryKind::suzanne, .slug = "suzanne"},
};
static_assert(capture_geometries.size() == geometry_kind_count);

[[nodiscard]] std::optional<GeometryKind>
geometry_kind_from_capture_slug(std::string_view slug) noexcept {
    for (const CaptureGeometry& geometry : capture_geometries) {
        if (geometry.slug == slug) {
            return geometry.kind;
        }
    }

    return std::nullopt;
}

[[nodiscard]] std::size_t rgb_byte_count(int width, int height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument{"Raw frame dimensions must be positive."};
    }

    constexpr std::size_t channel_count = 3;
    const auto unsigned_width = static_cast<std::size_t>(width);
    const auto unsigned_height = static_cast<std::size_t>(height);
    if (unsigned_width >
        std::numeric_limits<std::size_t>::max() / unsigned_height / channel_count) {
        throw std::invalid_argument{"Raw frame dimensions are too large."};
    }

    const std::size_t byte_count = unsigned_width * unsigned_height * channel_count;
    if (byte_count > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) {
        throw std::invalid_argument{"Raw frame is too large for file output."};
    }

    return byte_count;
}

class RawFrameWriter final {
  public:
    RawFrameWriter(const std::filesystem::path& output_file, int width, int height)
        : width_{width}, height_{height}, pixels_(rgb_byte_count(width, height)) {
        if (output_file.has_parent_path()) {
            std::filesystem::create_directories(output_file.parent_path());
        }

        output_.open(output_file, std::ios::binary | std::ios::trunc);
        if (!output_) {
            throw std::runtime_error{"Failed to open raw capture file: " + output_file.string()};
        }
    }

    ~RawFrameWriter() = default;

    RawFrameWriter(const RawFrameWriter&) = delete;
    RawFrameWriter& operator=(const RawFrameWriter&) = delete;
    RawFrameWriter(RawFrameWriter&&) = delete;
    RawFrameWriter& operator=(RawFrameWriter&&) = delete;

    void capture_back_buffer() {
        GLint previous_pack_alignment = 0;
        glGetIntegerv(GL_PACK_ALIGNMENT, &previous_pack_alignment);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadBuffer(GL_BACK);
        glReadPixels(0, 0, width_, height_, GL_RGB, GL_UNSIGNED_BYTE, pixels_.data());
        glPixelStorei(GL_PACK_ALIGNMENT, previous_pack_alignment);

        const GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            throw std::runtime_error{"OpenGL frame capture failed with error code " +
                                     std::to_string(error) + '.'};
        }

        output_.write(reinterpret_cast<const char*>(pixels_.data()),
                      static_cast<std::streamsize>(pixels_.size()));
        if (!output_) {
            throw std::runtime_error{"Failed while writing raw RGB24 capture data."};
        }
    }

    void finish() {
        output_.flush();
        if (!output_) {
            throw std::runtime_error{"Failed to finish raw RGB24 capture data."};
        }
    }

  private:
    int width_;
    int height_;
    std::ofstream output_;
    std::vector<unsigned char> pixels_;
};

} // namespace

void write_capture_geometry_slugs(std::ostream& output) {
    for (const CaptureGeometry& geometry : capture_geometries) {
        output << geometry.slug << '\n';
    }
}

void capture_animation(const FrameCaptureOptions& options) {
    const std::optional<GeometryKind> geometry =
        geometry_kind_from_capture_slug(options.geometry_slug);
    if (!geometry.has_value()) {
        throw std::invalid_argument{"Unknown geometry slug: " + options.geometry_slug};
    }

    const GlfwSession glfw_session{};
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    Window window = create_window(options.width, options.height, "quantum-wave-sphere capture");
    glfwMakeContextCurrent(window.get());

    const int loaded_version = load_opengl();
    initialize_opengl_diagnostics(loaded_version);

    configure_showcase_render_state();
    glfwSwapInterval(0);

    int framebuffer_width = 0;
    int framebuffer_height = 0;
    glfwGetFramebufferSize(window.get(), &framebuffer_width, &framebuffer_height);
    if (framebuffer_width != options.width || framebuffer_height != options.height) {
        throw std::runtime_error{
            "Hidden framebuffer size does not match the requested capture size."};
    }

    const FramebufferSize framebuffer_size{
        .width = framebuffer_width,
        .height = framebuffer_height,
    };
    auto wave_mesh = WaveMesh{make_showcase_geometries()};
    const auto view = make_showcase_view_matrix();
    const auto projection_matrix = make_showcase_projection_matrix(framebuffer_size);

    const double duration_seconds =
        std::min(WaveMesh::default_animation_period_seconds(), maximum_capture_duration_seconds);
    const auto rounded_frame_count =
        std::llround(duration_seconds * static_cast<double>(options.frames_per_second));
    const auto frame_count = static_cast<std::size_t>(std::max(1LL, rounded_frame_count));

    RawFrameWriter writer{std::filesystem::path{options.output_file}, framebuffer_width,
                          framebuffer_height};

    std::cout << "Capturing " << geometry_name(*geometry) << " as " << frame_count
              << " deterministic RGB24 frames over " << duration_seconds << " seconds.\n";

    for (std::size_t frame_index = 0; frame_index < frame_count; ++frame_index) {
        const double simulation_time =
            duration_seconds * static_cast<double>(frame_index) / static_cast<double>(frame_count);

        clear_showcase_frame(framebuffer_size);
        wave_mesh.draw_at_time(view, projection_matrix, *geometry, simulation_time);
        writer.capture_back_buffer();
    }

    writer.finish();
    std::cout << "Raw capture complete: " << options.output_file << '\n';
}

} // namespace qws
