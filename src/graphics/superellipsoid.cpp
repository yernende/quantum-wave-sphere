#include "graphics/superellipsoid.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/geometric.hpp>
#include <limits>
#include <stdexcept>

namespace qws {
namespace {

constexpr float vector_epsilon = 1.0e-12F;

[[nodiscard]] bool is_finite_positive(float value) noexcept {
    return std::isfinite(value) && value > 0.0F;
}

void validate_vertex_count(std::uint64_t vertex_count) {
    if (vertex_count > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument{"Superellipsoid has too many vertices for 32-bit indices."};
    }
}

struct CubeFace {
    glm::vec3 normal;
    glm::vec3 first_axis;
    glm::vec3 second_axis;
};

constexpr std::array cube_faces{
    CubeFace{.normal = {1.0F, 0.0F, 0.0F},
             .first_axis = {0.0F, 1.0F, 0.0F},
             .second_axis = {0.0F, 0.0F, 1.0F}},
    CubeFace{.normal = {-1.0F, 0.0F, 0.0F},
             .first_axis = {0.0F, 1.0F, 0.0F},
             .second_axis = {0.0F, 0.0F, -1.0F}},
    CubeFace{.normal = {0.0F, 1.0F, 0.0F},
             .first_axis = {0.0F, 0.0F, 1.0F},
             .second_axis = {1.0F, 0.0F, 0.0F}},
    CubeFace{.normal = {0.0F, -1.0F, 0.0F},
             .first_axis = {0.0F, 0.0F, 1.0F},
             .second_axis = {-1.0F, 0.0F, 0.0F}},
    CubeFace{.normal = {0.0F, 0.0F, 1.0F},
             .first_axis = {1.0F, 0.0F, 0.0F},
             .second_axis = {0.0F, 1.0F, 0.0F}},
    CubeFace{.normal = {0.0F, 0.0F, -1.0F},
             .first_axis = {-1.0F, 0.0F, 0.0F},
             .second_axis = {0.0F, 1.0F, 0.0F}},
};

[[nodiscard]] float signed_power_derivative(float value, float exponent) {
    const float magnitude = std::abs(value);
    if (magnitude <= vector_epsilon) {
        return 0.0F;
    }
    return std::copysign(std::pow(magnitude, exponent - 1.0F), value);
}

[[nodiscard]] MeshVertex make_vertex(const glm::vec3& cube_position, float exponent, float scale) {
    const float powered_sum = std::pow(std::abs(cube_position.x), exponent) +
                              std::pow(std::abs(cube_position.y), exponent) +
                              std::pow(std::abs(cube_position.z), exponent);
    const float radial_scale = scale * std::pow(powered_sum, -1.0F / exponent);
    const glm::vec3 position = cube_position * radial_scale;
    const glm::vec3 gradient{
        signed_power_derivative(position.x, exponent),
        signed_power_derivative(position.y, exponent),
        signed_power_derivative(position.z, exponent),
    };

    return MeshVertex{.position = position, .normal = glm::normalize(gradient)};
}

} // namespace

MeshData make_superellipsoid(const SuperellipsoidParameters& parameters) {
    if (parameters.face_segments == 0U) {
        throw std::invalid_argument{"Superellipsoid face segments must be positive."};
    }
    if (!is_finite_positive(parameters.exponent) ||
        !is_finite_positive(parameters.bounding_radius)) {
        throw std::invalid_argument{
            "Superellipsoid exponent and bounding radius must be finite and positive."};
    }

    const std::uint64_t edge_vertices = static_cast<std::uint64_t>(parameters.face_segments) + 1U;
    const std::uint64_t vertex_count = cube_faces.size() * edge_vertices * edge_vertices;
    validate_vertex_count(vertex_count);

    MeshData mesh;
    mesh.vertices.reserve(static_cast<std::size_t>(vertex_count));
    mesh.indices.reserve(static_cast<std::size_t>(parameters.face_segments) *
                         parameters.face_segments * cube_faces.size() * 6U);

    const float maximum_base_radius =
        parameters.exponent >= 2.0F ? std::pow(3.0F, 0.5F - 1.0F / parameters.exponent) : 1.0F;
    const float scale = parameters.bounding_radius / maximum_base_radius;
    const std::uint32_t vertices_per_edge = parameters.face_segments + 1U;

    for (const CubeFace& face : cube_faces) {
        const auto face_vertex_offset = static_cast<std::uint32_t>(mesh.vertices.size());

        for (std::uint32_t first_index = 0; first_index <= parameters.face_segments;
             ++first_index) {
            const float first_parameter = -1.0F + 2.0F * static_cast<float>(first_index) /
                                                      static_cast<float>(parameters.face_segments);

            for (std::uint32_t second_index = 0; second_index <= parameters.face_segments;
                 ++second_index) {
                const float second_parameter =
                    -1.0F + 2.0F * static_cast<float>(second_index) /
                                static_cast<float>(parameters.face_segments);
                const glm::vec3 cube_position = face.normal + face.first_axis * first_parameter +
                                                face.second_axis * second_parameter;
                mesh.vertices.push_back(make_vertex(cube_position, parameters.exponent, scale));
            }
        }

        for (std::uint32_t first_index = 0; first_index < parameters.face_segments; ++first_index) {
            for (std::uint32_t second_index = 0; second_index < parameters.face_segments;
                 ++second_index) {
                const std::uint32_t a =
                    face_vertex_offset + first_index * vertices_per_edge + second_index;
                const std::uint32_t b = a + vertices_per_edge;
                const std::uint32_t c = b + 1U;
                const std::uint32_t d = a + 1U;
                mesh.indices.insert(mesh.indices.end(), {a, b, c, a, c, d});
            }
        }
    }

    return mesh;
}

} // namespace qws
