#pragma once

#include "graphics/shader_program.hpp"

#include <glad/gl.h>
#include <glm/vec3.hpp>

namespace qws {

// Construction, drawing and destruction require a current OpenGL context.
class TriangleDemo final {
  public:
    TriangleDemo();
    ~TriangleDemo();

    TriangleDemo(const TriangleDemo&) = delete;
    TriangleDemo& operator=(const TriangleDemo&) = delete;
    TriangleDemo(TriangleDemo&&) = delete;
    TriangleDemo& operator=(TriangleDemo&&) = delete;

    void show_controls() noexcept;
    void draw() const noexcept;

  private:
    void release_geometry() noexcept;

    ShaderProgram program_;
    GLuint vertex_array_{0};
    GLuint vertex_buffer_{0};
    GLint tint_location_{-1};
    GLint scale_location_{-1};
    glm::vec3 tint_{0.15F, 0.72F, 0.95F};
    float scale_{1.0F};
};

} // namespace qws
