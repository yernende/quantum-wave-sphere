#include "graphics/torus.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace qws {
namespace {

constexpr float full_turn = 2.0F * std::numbers::pi_v<float>;

[[nodiscard]] bool is_finite_positive(float value) noexcept {
    return std::isfinite(value) && value > 0.0F;
}

void validate_vertex_count(std::uint64_t vertex_count) {
    if (vertex_count > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument{"Torus has too many vertices for 32-bit indices."};
    }
}

void append_wrapped_grid_indices(MeshData& mesh, std::uint32_t major_segments,
                                 std::uint32_t minor_segments) {
    mesh.indices.reserve(static_cast<std::size_t>(major_segments) * minor_segments * 6U);

    for (std::uint32_t major = 0; major < major_segments; ++major) {
        const std::uint32_t next_major = (major + 1U) % major_segments;

        for (std::uint32_t minor = 0; minor < minor_segments; ++minor) {
            const std::uint32_t next_minor = (minor + 1U) % minor_segments;
            const std::uint32_t a = major * minor_segments + minor;
            const std::uint32_t b = next_major * minor_segments + minor;
            const std::uint32_t c = next_major * minor_segments + next_minor;
            const std::uint32_t d = major * minor_segments + next_minor;

            mesh.indices.insert(mesh.indices.end(), {a, b, c, a, c, d});
        }
    }
}

} // namespace

MeshData make_torus(const TorusParameters& parameters) {
    if (parameters.major_segments < 3U || parameters.minor_segments < 3U) {
        throw std::invalid_argument{"Torus segment counts must be at least three."};
    }
    if (!is_finite_positive(parameters.major_radius) ||
        !is_finite_positive(parameters.minor_radius) ||
        parameters.major_radius <= parameters.minor_radius) {
        throw std::invalid_argument{
            "Torus radii must be finite and positive, with the major radius larger."};
    }

    const std::uint64_t vertex_count =
        static_cast<std::uint64_t>(parameters.major_segments) * parameters.minor_segments;
    validate_vertex_count(vertex_count);

    MeshData mesh;
    mesh.vertices.reserve(static_cast<std::size_t>(vertex_count));

    for (std::uint32_t major_index = 0; major_index < parameters.major_segments; ++major_index) {
        const float major_angle = full_turn * static_cast<float>(major_index) /
                                  static_cast<float>(parameters.major_segments);
        const float major_cosine = std::cos(major_angle);
        const float major_sine = std::sin(major_angle);

        for (std::uint32_t minor_index = 0; minor_index < parameters.minor_segments;
             ++minor_index) {
            const float minor_angle = full_turn * static_cast<float>(minor_index) /
                                      static_cast<float>(parameters.minor_segments);
            const float minor_cosine = std::cos(minor_angle);
            const float minor_sine = std::sin(minor_angle);
            const float radial_distance =
                parameters.major_radius + parameters.minor_radius * minor_cosine;
            const glm::vec3 normal{
                minor_cosine * major_cosine,
                minor_cosine * major_sine,
                minor_sine,
            };

            mesh.vertices.push_back(MeshVertex{
                .position = {radial_distance * major_cosine, radial_distance * major_sine,
                             parameters.minor_radius * minor_sine},
                .normal = normal,
            });
        }
    }

    append_wrapped_grid_indices(mesh, parameters.major_segments, parameters.minor_segments);
    return mesh;
}

} // namespace qws
