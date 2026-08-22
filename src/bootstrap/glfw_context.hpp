#pragma once

#include <memory>
#include <string_view>

struct GLFWwindow;

namespace qws {

class GlfwSession final {
  public:
    GlfwSession();
    ~GlfwSession();

    GlfwSession(const GlfwSession&) = delete;
    GlfwSession& operator=(const GlfwSession&) = delete;
    GlfwSession(GlfwSession&&) = delete;
    GlfwSession& operator=(GlfwSession&&) = delete;
};

struct WindowDeleter {
    void operator()(GLFWwindow* window) const noexcept;
};

using Window = std::unique_ptr<GLFWwindow, WindowDeleter>;

[[nodiscard]] Window create_window(int width, int height, std::string_view title);

// A current GLFW context is required because glfwGetProcAddress resolves functions for it.
[[nodiscard]] int load_opengl();

} // namespace qws
