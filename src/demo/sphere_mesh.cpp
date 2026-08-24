#include "demo/sphere_mesh.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <numbers>
#include <stdexcept>
#include <string_view>

namespace qws {
namespace {

constexpr std::string_view vertex_shader_source = R"glsl(#version 460 core
layout(location = 0) in vec3 aPosition;
layout(location = 0) out float vWaveHeight;

float angularDistance(vec3 a, vec3 b) {
    float cosine = clamp(dot(a, b), -1.0, 1.0);
    return acos(cosine);
}

uniform float uAmplitude;
uniform float uCycles;
uniform float uSpeed;
uniform float uDecay;
uniform float uTime;

float wave(
    vec3 direction,
    vec3 sourceDirection,
    float phaseOffset
) {
    float theta =
        angularDistance(
            direction,
            sourceDirection
        );

    float envelope =
        exp(-uDecay * theta);

    float phase =
        2.0 * uCycles * theta
        - uSpeed * uTime
        + phaseOffset;

    return
        uAmplitude *
        envelope *
        sin(phase);
}

const int maxWaves = 16;

uniform int uWaveCount;
uniform vec3 uSourceDirections[maxWaves];
uniform float uPhaseOffsets[maxWaves];

float evaluateWaveField(vec3 direction)
{
    float result = 0.0;
    int waveCount = clamp(uWaveCount, 1, maxWaves);

    for (int i = 0; i < maxWaves; ++i) {
        if (i >= waveCount) {
            break;
        }

        result += wave(
            direction,
            uSourceDirections[i],
            uPhaseOffsets[i]
        );
    }

    return result;
}

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    vec3 direction = normalize(aPosition);

    float height = evaluateWaveField(direction);
    vWaveHeight = height;

    vec3 displacedPosition =
        direction * (1.0 + height);

    gl_Position =
        uProjection *
        uView *
        uModel *
        vec4(displacedPosition, 1.0);
}
)glsl";

constexpr std::string_view fragment_shader_source = R"glsl(#version 460 core
layout(location = 0) in float vWaveHeight;
layout(location = 0) out vec4 fragmentColor;

uniform float uAmplitude;
uniform int uWaveCount;

const vec3 backgroundColor = vec3(0.025, 0.035, 0.055);
const vec3 negativeColor = vec3(0.95, 0.10, 0.005);
const vec3 positiveColor = vec3(0.24, 0.01, 0.90);

void main() {
    float maximumHeight =
        float(max(uWaveCount, 1)) *
        max(uAmplitude, 0.0001);

    float normalizedWave =
        clamp(
            vWaveHeight / maximumHeight,
            -1.0,
            1.0
        );

    float magnitude = abs(normalizedWave);
    float visibility = smoothstep(0.03, 0.40, magnitude);

    vec3 waveColor =
        normalizedWave < 0.0
            ? negativeColor
            : positiveColor;

    vec3 color = mix(
        backgroundColor,
        waveColor,
        visibility
    );

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
        amplitude_ = glGetUniformLocation(program_.id(), "uAmplitude");
        cycles_ = glGetUniformLocation(program_.id(), "uCycles");
        speed_ = glGetUniformLocation(program_.id(), "uSpeed");
        decay_ = glGetUniformLocation(program_.id(), "uDecay");
        time_ = glGetUniformLocation(program_.id(), "uTime");
        wave_count_ = glGetUniformLocation(program_.id(), "uWaveCount");
        source_directions_location_ = glGetUniformLocation(program_.id(), "uSourceDirections[0]");
        phase_offsets_location_ = glGetUniformLocation(program_.id(), "uPhaseOffsets[0]");

        if (model_location_ < 0 || view_location_ < 0 || projection_location_ < 0 ||
            amplitude_ < 0 || cycles_ < 0 || speed_ < 0 || decay_ < 0 || time_ < 0 ||
            wave_count_ < 0 || source_directions_location_ < 0 || phase_offsets_location_ < 0) {
            throw std::runtime_error{"Required sphere shader uniforms were optimized out."};
        }

        regenerate_wave_sources();
        previous_frame_time_ = glfwGetTime();
    } catch (...) {
        release_geometry();
        throw;
    }
}

SphereMesh::~SphereMesh() {
    release_geometry();
}

void SphereMesh::regenerate_wave_sources() noexcept {
    constexpr float full_turn = 2.0F * std::numbers::pi_v<float>;
    constexpr float golden_angle = full_turn * (1.0F - 1.0F / std::numbers::phi_v<float>);

    const auto wave_count = static_cast<float>(setting_wave_count_);

    for (int index = 0; index < setting_wave_count_; ++index) {
        const auto source_index = static_cast<std::size_t>(index);
        const float normalized_index = (static_cast<float>(index) + 0.5F) / wave_count;
        const float y = 1.0F - (2.0F * normalized_index);
        const float radius = std::sqrt(std::max(0.0F, 1.0F - y * y));
        const float azimuth = golden_angle * static_cast<float>(index);

        source_directions_[source_index] =
            glm::vec3{radius * std::cos(azimuth), y, radius * std::sin(azimuth)};
        phase_offsets_[source_index] = full_turn * static_cast<float>(index) / wave_count;
    }

    wave_sources_dirty_ = true;
}

constexpr float sphere_rotation_speed = 0.25F;

// The names carry fixed transform roles; strong wrapper types would add noise here.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void SphereMesh::draw(const glm::mat4& view_matrix, const glm::mat4& projection_matrix) noexcept {
    const double current_frame_time = glfwGetTime();
    const double delta_time = current_frame_time - previous_frame_time_;
    previous_frame_time_ = current_frame_time;

    if (!setting_paused_) {
        simulation_time_ += delta_time;
        rotation_angle_radians_ += static_cast<double>(sphere_rotation_speed) * delta_time;
    }

    const auto simulation_time = static_cast<float>(simulation_time_);
    const auto rotation = static_cast<float>(rotation_angle_radians_);
    const glm::mat4 model_matrix =
        glm::rotate(glm::mat4{1.0F}, rotation, glm::vec3{0.0F, 1.0F, 0.0F});

    glUseProgram(program_.id());

    glUniformMatrix4fv(model_location_, 1, GL_FALSE, glm::value_ptr(model_matrix));
    glUniformMatrix4fv(view_location_, 1, GL_FALSE, glm::value_ptr(view_matrix));
    glUniformMatrix4fv(projection_location_, 1, GL_FALSE, glm::value_ptr(projection_matrix));
    glUniform1f(amplitude_, setting_amplitude_);
    glUniform1f(cycles_, setting_cycles_);
    glUniform1f(speed_, setting_speed_);
    glUniform1f(decay_, setting_decay_);
    glUniform1f(time_, simulation_time);

    if (wave_sources_dirty_) {
        const auto wave_count = static_cast<GLsizei>(setting_wave_count_);

        glUniform1i(wave_count_, setting_wave_count_);
        glUniform3fv(source_directions_location_, wave_count,
                     glm::value_ptr(source_directions_.front()));
        glUniform1fv(phase_offsets_location_, wave_count, phase_offsets_.data());

        wave_sources_dirty_ = false;
    }

    glBindVertexArray(vertex_array_);

    glPolygonMode(GL_FRONT_AND_BACK, setting_wireframe_ ? GL_LINE : GL_FILL);
    glDrawElements(GL_TRIANGLES, indices_size_, GL_UNSIGNED_INT, nullptr);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void SphereMesh::show_controls() noexcept {
    if (ImGui::SliderInt("Sources", &setting_wave_count_, 1, max_wave_count)) {
        regenerate_wave_sources();
    }

    ImGui::SliderFloat("Amplitude", &setting_amplitude_, 0.0f, 0.25f);
    ImGui::SliderFloat("Cycles", &setting_cycles_, 1.0f, 40.0f);
    ImGui::SliderFloat("Speed", &setting_speed_, 0.0f, 5.0f);
    ImGui::SliderFloat("Decay", &setting_decay_, 0.0f, 5.0f);
    ImGui::Checkbox("Pause", &setting_paused_);
    ImGui::Checkbox("Wireframe", &setting_wireframe_);
}

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
