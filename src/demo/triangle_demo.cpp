#include "demo/triangle_demo.hpp"

#include <array>
#include <cstddef>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <stdexcept>
#include <string_view>

namespace qws {
namespace {

constexpr std::string_view vertex_shader_source = R"glsl(#version 460 core
layout(location = 0) in vec2 aPosition;

uniform float uScale;

void main() {
    gl_Position = vec4(aPosition * uScale, 0.0, 1.0);
}
)glsl";

constexpr std::string_view fragment_shader_source = R"glsl(#version 460 core
layout(location = 0) out vec4 fragmentColor;

uniform vec3 uTint;

void main() {
    fragmentColor = vec4(uTint, 1.0);
}
)glsl";

} // namespace

TriangleDemo::TriangleDemo() : program_{vertex_shader_source, fragment_shader_source} {
    constexpr std::array vertices{
        -0.65F, -0.55F, 0.65F, -0.55F, 0.0F, 0.65F,
    };

    try {
        glGenVertexArrays(1, &vertex_array_);
        glGenBuffers(1, &vertex_buffer_);
        if (vertex_array_ == 0 || vertex_buffer_ == 0) {
            throw std::runtime_error{"OpenGL failed to allocate triangle geometry."};
        }

        glBindVertexArray(vertex_array_);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(vertices)), vertices.data(),
                     GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(2 * sizeof(float)),
                              nullptr);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        tint_location_ = glGetUniformLocation(program_.id(), "uTint");
        scale_location_ = glGetUniformLocation(program_.id(), "uScale");
        if (tint_location_ < 0 || scale_location_ < 0) {
            throw std::runtime_error{"Required triangle shader uniforms were optimized out."};
        }
    } catch (...) {
        release_geometry();
        throw;
    }
}

TriangleDemo::~TriangleDemo() {
    release_geometry();
}

void TriangleDemo::show_controls() noexcept {
    ImGui::Begin("Bootstrap controls");
    ImGui::TextUnformatted("The wave sphere is intentionally not implemented yet.");
    ImGui::Separator();
    ImGui::SliderFloat("Triangle scale", &scale_, 0.25F, 1.25F);
    ImGui::ColorEdit3("Triangle color", glm::value_ptr(tint_));
    ImGui::End();
}

void TriangleDemo::draw() const noexcept {
    glUseProgram(program_.id());
    glUniform3fv(tint_location_, 1, glm::value_ptr(tint_));
    glUniform1f(scale_location_, scale_);
    glBindVertexArray(vertex_array_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void TriangleDemo::release_geometry() noexcept {
    if (vertex_buffer_ != 0) {
        glDeleteBuffers(1, &vertex_buffer_);
        vertex_buffer_ = 0;
    }
    if (vertex_array_ != 0) {
        glDeleteVertexArrays(1, &vertex_array_);
        vertex_array_ = 0;
    }
}

} // namespace qws
