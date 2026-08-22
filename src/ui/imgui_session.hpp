#pragma once

struct GLFWwindow;
struct ImGuiContext;

namespace qws {

class ImGuiSession final {
  public:
    explicit ImGuiSession(GLFWwindow* window);
    ~ImGuiSession();

    ImGuiSession(const ImGuiSession&) = delete;
    ImGuiSession& operator=(const ImGuiSession&) = delete;
    ImGuiSession(ImGuiSession&&) = delete;
    ImGuiSession& operator=(ImGuiSession&&) = delete;

    void begin_frame() const noexcept;
    void render() const noexcept;

  private:
    ImGuiContext* context_{nullptr};
};

} // namespace qws
