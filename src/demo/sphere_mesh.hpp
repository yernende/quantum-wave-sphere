#pragma once

#include "graphics/icosphere.hpp"
#include "graphics/shader_program.hpp"

#include <glad/gl.h>
#include <glm/mat4x4.hpp>

namespace qws {
class SphereMesh final {
  public:
    explicit SphereMesh(MeshData mesh_data);
    ~SphereMesh();

    SphereMesh(const SphereMesh&) = delete;
    SphereMesh& operator=(const SphereMesh&) = delete;
    SphereMesh(SphereMesh&&) = delete;
    SphereMesh& operator=(SphereMesh&&) = delete;

    void show_controls() noexcept;

    // The names carry fixed transform roles; strong wrapper types would add noise here.
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    void draw(const glm::mat4& model, const glm::mat4& view,
              const glm::mat4& projection) const noexcept;

  private:
    void release_geometry() noexcept;

    ShaderProgram program_;

    GLuint vertex_array_{0};
    GLuint element_buffer_{0};
    GLuint vertex_buffer_{0};
    GLint model_location_{-1};
    GLint view_location_{-1};
    GLint projection_location_{-1};

    GLsizei indices_size_{0};
};
} // namespace qws
