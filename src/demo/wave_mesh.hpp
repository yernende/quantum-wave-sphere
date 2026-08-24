#pragma once

#include "graphics/geometry_catalog.hpp"
#include "graphics/shader_program.hpp"

#include <array>
#include <glad/gl.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace qws {

class WaveMesh final {
  public:
    explicit WaveMesh(std::array<MeshData, geometry_kind_count> mesh_data);
    ~WaveMesh() = default;

    WaveMesh(const WaveMesh&) = delete;
    WaveMesh& operator=(const WaveMesh&) = delete;
    WaveMesh(WaveMesh&&) = delete;
    WaveMesh& operator=(WaveMesh&&) = delete;

    void show_controls() noexcept;

    // The names carry fixed transform roles; strong wrapper types would add noise here.
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    void draw(const glm::mat4& view, const glm::mat4& projection) noexcept;

  private:
    class GpuGeometry final {
      public:
        GpuGeometry() = default;
        ~GpuGeometry();

        GpuGeometry(const GpuGeometry&) = delete;
        GpuGeometry& operator=(const GpuGeometry&) = delete;
        GpuGeometry(GpuGeometry&&) = delete;
        GpuGeometry& operator=(GpuGeometry&&) = delete;

        void upload(const MeshData& mesh_data);
        void bind() const noexcept;
        [[nodiscard]] GLsizei index_count() const noexcept;

      private:
        void release() noexcept;

        GLuint vertex_array_{0};
        GLuint element_buffer_{0};
        GLuint vertex_buffer_{0};
        GLsizei index_count_{0};
    };

    static constexpr int max_wave_count = 16;

    void regenerate_wave_sources() noexcept;

    ShaderProgram program_;
    std::array<GpuGeometry, geometry_kind_count> geometries_;

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
    int setting_geometry_index_{0};
    int setting_wave_count_{2};
    float setting_amplitude_{0.001F};
    float setting_cycles_{3.0F};
    float setting_speed_{1.0F};
    float setting_decay_{0.5F};
    bool setting_paused_{false};
    bool setting_wireframe_{false};

    // Animation clock
    double previous_frame_time_{0.0};
    double simulation_time_{0.0};
    double rotation_angle_radians_{0.0};
};

} // namespace qws
