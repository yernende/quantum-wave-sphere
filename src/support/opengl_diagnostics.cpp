#include "support/opengl_diagnostics.hpp"

#include <glad/gl.h>
#include <iostream>
#include <string_view>

namespace qws {
namespace {

#ifndef NDEBUG
// OpenGL fixes this callback signature; its adjacent scalar parameters cannot be reordered.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void GLAD_API_PTR open_gl_debug_callback([[maybe_unused]] GLenum source,
                                         [[maybe_unused]] GLenum type, [[maybe_unused]] GLuint id,
                                         GLenum severity, [[maybe_unused]] GLsizei length,
                                         const GLchar* message,
                                         [[maybe_unused]] const void* user_parameter) {
    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
        std::cerr << "OpenGL debug: " << message << '\n';
    }
}
#endif

[[nodiscard]] std::string_view open_gl_string(GLenum name) {
    const GLubyte* value = glGetString(name);
    if (value == nullptr) {
        return "<unavailable>";
    }
    return reinterpret_cast<const char*>(value);
}

} // namespace

void initialize_opengl_diagnostics(int loaded_version) {
    std::cout << "OpenGL vendor:   " << open_gl_string(GL_VENDOR) << '\n'
              << "OpenGL renderer: " << open_gl_string(GL_RENDERER) << '\n'
              << "OpenGL version:  " << open_gl_string(GL_VERSION) << '\n'
              << "GLSL version:    " << open_gl_string(GL_SHADING_LANGUAGE_VERSION) << '\n'
              << "GLAD2 loaded:    " << GLAD_VERSION_MAJOR(loaded_version) << '.'
              << GLAD_VERSION_MINOR(loaded_version) << '\n';

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(open_gl_debug_callback, nullptr);
#endif
}

} // namespace qws
