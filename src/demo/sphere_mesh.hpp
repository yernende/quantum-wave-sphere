#pragma once

#include "graphics/icosphere.hpp"
#include "graphics/shader_program.hpp"

#include <array>
#include <glad/gl.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

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
    void draw(const glm::mat4& view, const glm::mat4& projection) noexcept;

  private:
    static constexpr int max_wave_count = 16;

    void regenerate_wave_sources() noexcept;
    void release_geometry() noexcept;

    ShaderProgram program_;

    GLuint vertex_array_{0};
    GLuint element_buffer_{0};
    GLuint vertex_buffer_{0};
    GLsizei indices_size_{0};

    // Uniforms
    GLint model_location_{-1};
    GLint view_location_{-1};
    GLint projection_location_{-1};
    GLint amplitude_{-1};
    GLint cycles_{-1};
    GLint speed_{-1};
    GLint decay_{-1};
    GLint time_{-1};
    GLint wave_count_{-1};
    GLint source_directions_location_{-1};
    GLint phase_offsets_location_{-1};

    // Procedural wave sources
    std::array<glm::vec3, max_wave_count> source_directions_{};
    std::array<float, max_wave_count> phase_offsets_{};
    bool wave_sources_dirty_{true};

    // ImGui settings
    int setting_wave_count_{2};
    float setting_amplitude_{0.001F};
    float setting_cycles_{3.0F};
    float setting_speed_{1.0F};
    float setting_decay_{0.5F};
    bool setting_paused_{false};
    bool setting_wireframe_{true};

    // Animation clock
    double previous_frame_time_{0.0};
    double simulation_time_{0.0};
    double rotation_angle_radians_{0.0};
};
} // namespace qws
