#include "ui/imgui_session.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdexcept>

namespace qws {

ImGuiSession::ImGuiSession(GLFWwindow* window) {
    if (window == nullptr || glfwGetCurrentContext() != window) {
        throw std::runtime_error{"Dear ImGui requires its window's OpenGL context to be current."};
    }

    IMGUI_CHECKVERSION();
    context_ = ImGui::CreateContext();
    if (context_ == nullptr) {
        throw std::runtime_error{"Failed to create the Dear ImGui context."};
    }

    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        ImGui::DestroyContext(context_);
        throw std::runtime_error{"Failed to initialize Dear ImGui's GLFW backend."};
    }

    if (!ImGui_ImplOpenGL3_Init("#version 460 core")) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(context_);
        throw std::runtime_error{"Failed to initialize Dear ImGui's OpenGL backend."};
    }
}

ImGuiSession::~ImGuiSession() {
    ImGui::SetCurrentContext(context_);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext(context_);
}

void ImGuiSession::begin_frame() const noexcept {
    ImGui::SetCurrentContext(context_);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiSession::render() const noexcept {
    ImGui::SetCurrentContext(context_);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace qws
