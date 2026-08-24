#include "bootstrap/glfw_context.hpp"
#include "capture/animation_capture.hpp"
#include "demo/showcase_scene.hpp"
#include "demo/wave_mesh.hpp"
#include "graphics/geometry_catalog.hpp"
#include "support/app_options.hpp"
#include "support/opengl_diagnostics.hpp"
#include "support/smoke_test.hpp"
#include "ui/imgui_session.hpp"

#include <GLFW/glfw3.h>
#include <exception>
#include <iostream>
#include <stdexcept>

namespace {

constexpr int window_width = 1280;
constexpr int window_height = 720;

int run(qws::RunMode run_mode) {
    const qws::GlfwSession glfw_session{};
    qws::Window window = qws::create_window(window_width, window_height, "quantum-wave-sphere");
    glfwMakeContextCurrent(window.get());

    const int loaded_version = qws::load_opengl();
    qws::initialize_opengl_diagnostics(loaded_version);

    qws::configure_showcase_render_state();

    qws::SmokeTest smoke_test{run_mode};
    glfwSwapInterval(smoke_test.enabled() ? 0 : 1);

    // These objects must be destroyed while their OpenGL context is still current.
    const qws::ImGuiSession imgui{window.get()};

    auto wave_mesh = qws::WaveMesh{qws::make_showcase_geometries()};
    const auto view = qws::make_showcase_view_matrix();

    while (glfwWindowShouldClose(window.get()) == GLFW_FALSE) {
        glfwPollEvents();
        imgui.begin_frame();
        wave_mesh.show_controls();

        int framebuffer_width = 0;
        int framebuffer_height = 0;
        glfwGetFramebufferSize(window.get(), &framebuffer_width, &framebuffer_height);
        const qws::FramebufferSize framebuffer_size{
            .width = framebuffer_width,
            .height = framebuffer_height,
        };
        qws::clear_showcase_frame(framebuffer_size);

        if (framebuffer_width > 0 && framebuffer_height > 0) {
            const auto projection_matrix = qws::make_showcase_projection_matrix(framebuffer_size);

            wave_mesh.draw(view, projection_matrix);
        }

        imgui.render();

        smoke_test.frame_rendered();
        glfwSwapBuffers(window.get());

        if (smoke_test.complete()) {
            glfwSetWindowShouldClose(window.get(), GLFW_TRUE);
        }
    }

    smoke_test.verify_complete();
    return 0;
}

} // namespace

// Project code throws std::exception types, and standard streams keep their non-throwing default.
// NOLINTNEXTLINE(bugprone-exception-escape)
int main(int argc, char** argv) {
    try {
        qws::AppOptions options{};
        try {
            options = qws::parse_app_options(argc, argv);
        } catch (const std::invalid_argument& exception) {
            std::cerr << exception.what() << '\n' << qws::usage();
            return 2;
        }

        if (options.show_help) {
            std::cout << qws::usage();
            return 0;
        }

        if (options.list_geometries) {
            qws::write_capture_geometry_slugs(std::cout);
            return 0;
        }

        if (options.frame_capture.has_value()) {
            qws::capture_animation(*options.frame_capture);
            return 0;
        }

        return run(options.run_mode);
    } catch (const std::exception& exception) {
        std::cerr << "Fatal error: " << exception.what() << '\n';
        return 1;
    }
}
