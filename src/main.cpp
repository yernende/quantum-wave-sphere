#include "bootstrap/glfw_context.hpp"
#include "demo/sphere_mesh.hpp"
#include "graphics/icosphere.hpp"
#include "support/app_options.hpp"
#include "support/opengl_diagnostics.hpp"
#include "support/smoke_test.hpp"
#include "ui/imgui_session.hpp"

#include <GLFW/glfw3.h>
#include <exception>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <iostream>
#include <stdexcept>

namespace {

constexpr int window_width = 1280;
constexpr int window_height = 720;
constexpr float sphere_rotation_speed = 0.25F;

int run(const qws::AppOptions& options) {
    const qws::GlfwSession glfw_session{};
    qws::Window window = qws::create_window(window_width, window_height, "quantum-wave-sphere");
    glfwMakeContextCurrent(window.get());

    const int loaded_version = qws::load_opengl();
    qws::initialize_opengl_diagnostics(loaded_version);

    glEnable(GL_DEPTH_TEST);

    qws::SmokeTest smoke_test{options.run_mode};
    glfwSwapInterval(smoke_test.enabled() ? 0 : 1);

    // These objects must be destroyed while their OpenGL context is still current.
    const qws::ImGuiSession imgui{window.get()};
    // qws::TriangleDemo triangle{};

    auto icosphere = qws::make_icosphere(4);
    auto sphere_mesh = qws::SphereMesh{icosphere};
    const glm::mat4 view =
        glm::lookAt(glm::vec3{0.0F, 0.0F, 3.0F}, glm::vec3{0.0F}, glm::vec3{0.0F, 1.0F, 0.0F});

    while (glfwWindowShouldClose(window.get()) == GLFW_FALSE) {
        glfwPollEvents();
        imgui.begin_frame();
        sphere_mesh.show_controls();

        int framebuffer_width = 0;
        int framebuffer_height = 0;
        glfwGetFramebufferSize(window.get(), &framebuffer_width, &framebuffer_height);
        glViewport(0, 0, framebuffer_width, framebuffer_height);
        glClearColor(0.025F, 0.035F, 0.055F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (framebuffer_width > 0 && framebuffer_height > 0) {
            const float rotation = static_cast<float>(glfwGetTime()) * sphere_rotation_speed;

            const glm::mat4 model_matrix =
                glm::rotate(glm::mat4{1.0F}, rotation, glm::vec3{0.0F, 1.0F, 0.0F});

            const glm::mat4 projection_matrix = glm::perspective(
                glm::radians(45.0F),
                static_cast<float>(framebuffer_width) / static_cast<float>(framebuffer_height),
                0.1F, 100.0F);

            sphere_mesh.draw(model_matrix, view, projection_matrix);
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

        return run(options);
    } catch (const std::exception& exception) {
        std::cerr << "Fatal error: " << exception.what() << '\n';
        return 1;
    }
}
