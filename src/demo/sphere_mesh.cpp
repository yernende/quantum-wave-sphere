#include "demo/sphere_mesh.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <string_view>

namespace qws {
namespace {

constexpr std::string_view vertex_shader_source = R"glsl(#version 460 core
layout(location = 0) in vec3 aPosition;
layout(location = 0) out vec3 vObjectPosition;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    vObjectPosition = aPosition;
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}
)glsl";

constexpr std::string_view fragment_shader_source = R"glsl(#version 460 core
layout(location = 0) in vec3 vObjectPosition;
layout(location = 0) out vec4 fragmentColor;

void main() {
    const vec3 direction = normalize(vObjectPosition);
    const vec3 color = 0.5 + 0.5 * direction;
    fragmentColor = vec4(color, 1.0);
}
)glsl";

} // namespace

SphereMesh::SphereMesh(MeshData mesh_data)
    : program_{vertex_shader_source, fragment_shader_source} {
    try {
        glCreateVertexArrays(1, &vertex_array_);
        glCreateBuffers(1, &vertex_buffer_);
        glCreateBuffers(1, &element_buffer_);

        indices_size_ = static_cast<GLsizei>(mesh_data.indices.size());

        glNamedBufferStorage(
            vertex_buffer_, static_cast<GLsizeiptr>(mesh_data.positions.size() * sizeof(glm::vec3)),
            mesh_data.positions.data(), 0);

        glNamedBufferStorage(element_buffer_,
                             static_cast<GLsizeiptr>(indices_size_ * sizeof(std::uint32_t)),
                             mesh_data.indices.data(), 0);

        glVertexArrayVertexBuffer(vertex_array_, 0, vertex_buffer_, 0, sizeof(glm::vec3));
        glEnableVertexArrayAttrib(vertex_array_, 0);
        glVertexArrayAttribFormat(vertex_array_, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vertex_array_, 0, 0);

        glVertexArrayElementBuffer(vertex_array_, element_buffer_);

        model_location_ = glGetUniformLocation(program_.id(), "uModel");
        view_location_ = glGetUniformLocation(program_.id(), "uView");
        projection_location_ = glGetUniformLocation(program_.id(), "uProjection");

        if (model_location_ < 0 || view_location_ < 0 || projection_location_ < 0) {
            throw std::runtime_error{"Required sphere shader uniforms were optimized out."};
        }
    } catch (...) {
        release_geometry();
        throw;
    }
}

SphereMesh::~SphereMesh() {
    release_geometry();
}

// The names carry fixed transform roles; strong wrapper types would add noise here.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void SphereMesh::draw(const glm::mat4& model_matrix, const glm::mat4& view_matrix,
                      const glm::mat4& projection_matrix) const noexcept {
    glUseProgram(program_.id());

    glUniformMatrix4fv(model_location_, 1, GL_FALSE, glm::value_ptr(model_matrix));
    glUniformMatrix4fv(view_location_, 1, GL_FALSE, glm::value_ptr(view_matrix));
    glUniformMatrix4fv(projection_location_, 1, GL_FALSE, glm::value_ptr(projection_matrix));

    glBindVertexArray(vertex_array_);
    glDrawElements(GL_TRIANGLES, indices_size_, GL_UNSIGNED_INT, nullptr);
}

void SphereMesh::show_controls() noexcept {}

void SphereMesh::release_geometry() noexcept {
    if (vertex_buffer_ != 0) {
        glDeleteBuffers(1, &vertex_buffer_);
        vertex_buffer_ = 0;
    }

    if (element_buffer_ != 0) {
        glDeleteBuffers(1, &element_buffer_);
        element_buffer_ = 0;
    }

    if (vertex_array_ != 0) {
        glDeleteVertexArrays(1, &vertex_array_);
        vertex_array_ = 0;
    }
}
} // namespace qws
