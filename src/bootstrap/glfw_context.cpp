#include "bootstrap/glfw_context.hpp"

#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <iostream>
#include <stdexcept>
#include <string>

namespace qws {
namespace {

void glfw_error_callback(int error_code, const char* description) {
    std::cerr << "GLFW error " << error_code << ": " << description << '\n';
}

[[nodiscard]] GLADapiproc glfw_load_proc(const char* name) {
    return reinterpret_cast<GLADapiproc>(glfwGetProcAddress(name));
}

} // namespace

GlfwSession::GlfwSession() {
    glfwSetErrorCallback(glfw_error_callback);
#if defined(__linux__)
    // GLFW must use X11 even when the desktop session itself runs through Wayland/XWayland.
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error{"Failed to initialize GLFW."};
    }
}

GlfwSession::~GlfwSession() {
    glfwTerminate();
}

void WindowDeleter::operator()(GLFWwindow* window) const noexcept {
    glfwDestroyWindow(window);
}

Window create_window(int width, int height, std::string_view title) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    const std::string null_terminated_title{title};
    Window window{glfwCreateWindow(width, height, null_terminated_title.c_str(), nullptr, nullptr)};
    if (!window) {
        throw std::runtime_error{"Failed to create an OpenGL 4.6 Core window."};
    }

    return window;
}

int load_opengl() {
    if (glfwGetCurrentContext() == nullptr) {
        throw std::runtime_error{"An OpenGL context must be current before loading GLAD2."};
    }

    const int loaded_version = gladLoadGL(glfw_load_proc);
    if (loaded_version == 0) {
        throw std::runtime_error{"GLAD2 failed to load OpenGL functions."};
    }

    const int major = GLAD_VERSION_MAJOR(loaded_version);
    const int minor = GLAD_VERSION_MINOR(loaded_version);
    if (major < 4 || (major == 4 && minor < 6)) {
        throw std::runtime_error{"The active context does not provide OpenGL 4.6."};
    }

    return loaded_version;
}

} // namespace qws
