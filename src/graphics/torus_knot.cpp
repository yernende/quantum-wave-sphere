#include "graphics/torus_knot.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/geometric.hpp>
#include <limits>
#include <numbers>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace qws {
namespace {

constexpr float full_turn = 2.0F * std::numbers::pi_v<float>;
constexpr float vector_epsilon = 1.0e-12F;

[[nodiscard]] bool is_finite_positive(float value) noexcept {
    return std::isfinite(value) && value > 0.0F;
}

void validate_vertex_count(std::uint64_t vertex_count) {
    if (vertex_count > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument{"Torus knot has too many vertices for 32-bit indices."};
    }
}

void append_wrapped_grid_indices(MeshData& mesh, std::uint32_t path_segments,
                                 std::uint32_t tube_segments) {
    mesh.indices.reserve(static_cast<std::size_t>(path_segments) * tube_segments * 6U);

    for (std::uint32_t path = 0; path < path_segments; ++path) {
        const std::uint32_t next_path = (path + 1U) % path_segments;

        for (std::uint32_t tube = 0; tube < tube_segments; ++tube) {
            const std::uint32_t next_tube = (tube + 1U) % tube_segments;
            const std::uint32_t a = path * tube_segments + tube;
            const std::uint32_t b = next_path * tube_segments + tube;
            const std::uint32_t c = next_path * tube_segments + next_tube;
            const std::uint32_t d = path * tube_segments + next_tube;

            mesh.indices.insert(mesh.indices.end(), {a, b, c, a, c, d});
        }
    }
}

[[nodiscard]] glm::vec3 knot_position(const TorusKnotParameters& parameters, float parameter) {
    const float path_angle = static_cast<float>(parameters.winding_p) * parameter;
    const float knot_angle = static_cast<float>(parameters.winding_q) * parameter;
    const float radial_distance =
        parameters.major_radius + parameters.knot_radius * std::cos(knot_angle);

    return glm::vec3{
        radial_distance * std::cos(path_angle),
        radial_distance * std::sin(path_angle),
        parameters.knot_radius * std::sin(knot_angle),
    };
}

[[nodiscard]] glm::vec3 knot_tangent(const TorusKnotParameters& parameters, float parameter) {
    const float path_winding = static_cast<float>(parameters.winding_p);
    const float knot_winding = static_cast<float>(parameters.winding_q);
    const float path_angle = path_winding * parameter;
    const float knot_angle = knot_winding * parameter;
    const float radial_distance =
        parameters.major_radius + parameters.knot_radius * std::cos(knot_angle);
    const float radial_derivative = -parameters.knot_radius * knot_winding * std::sin(knot_angle);

    return glm::normalize(glm::vec3{
        radial_derivative * std::cos(path_angle) -
            radial_distance * path_winding * std::sin(path_angle),
        radial_derivative * std::sin(path_angle) +
            radial_distance * path_winding * std::cos(path_angle),
        parameters.knot_radius * knot_winding * std::cos(knot_angle),
    });
}

[[nodiscard]] glm::vec3 least_aligned_axis(const glm::vec3& direction) noexcept {
    const glm::vec3 magnitude = glm::abs(direction);
    if (magnitude.x <= magnitude.y && magnitude.x <= magnitude.z) {
        return glm::vec3{1.0F, 0.0F, 0.0F};
    }
    if (magnitude.y <= magnitude.z) {
        return glm::vec3{0.0F, 1.0F, 0.0F};
    }
    return glm::vec3{0.0F, 0.0F, 1.0F};
}

[[nodiscard]] glm::vec3 rotate_around_axis(const glm::vec3& value, const glm::vec3& axis,
                                           float angle) noexcept {
    const float sine = std::sin(angle);
    const float cosine = std::cos(angle);
    return value * cosine + glm::cross(axis, value) * sine +
           axis * glm::dot(axis, value) * (1.0F - cosine);
}

[[nodiscard]] glm::vec3 transport_normal(const glm::vec3& normal, const glm::vec3& from_tangent,
                                         const glm::vec3& to_tangent) noexcept {
    const glm::vec3 rotation_axis = glm::cross(from_tangent, to_tangent);
    const float sine = glm::length(rotation_axis);
    const float cosine = std::clamp(glm::dot(from_tangent, to_tangent), -1.0F, 1.0F);

    glm::vec3 transported = normal;
    if (sine > vector_epsilon) {
        transported = rotate_around_axis(normal, rotation_axis / sine, std::atan2(sine, cosine));
    } else if (cosine < 0.0F) {
        const glm::vec3 fallback_axis =
            glm::normalize(glm::cross(from_tangent, least_aligned_axis(from_tangent)));
        transported = rotate_around_axis(normal, fallback_axis, std::numbers::pi_v<float>);
    }

    transported -= to_tangent * glm::dot(transported, to_tangent);
    return glm::normalize(transported);
}

[[nodiscard]] std::vector<glm::vec3>
make_rotation_minimizing_normals(const std::vector<glm::vec3>& tangents) {
    std::vector<glm::vec3> normals(tangents.size());
    normals.front() =
        glm::normalize(glm::cross(tangents.front(), least_aligned_axis(tangents.front())));

    for (std::size_t index = 1; index < tangents.size(); ++index) {
        normals[index] = transport_normal(normals[index - 1], tangents[index - 1], tangents[index]);
    }

    const glm::vec3 closing_normal =
        transport_normal(normals.back(), tangents.back(), tangents.front());
    const float closing_twist =
        std::atan2(glm::dot(tangents.front(), glm::cross(closing_normal, normals.front())),
                   std::clamp(glm::dot(closing_normal, normals.front()), -1.0F, 1.0F));
    const float sample_count = static_cast<float>(normals.size());

    for (std::size_t index = 1; index < normals.size(); ++index) {
        const float correction = closing_twist * static_cast<float>(index) / sample_count;
        normals[index] =
            glm::normalize(rotate_around_axis(normals[index], tangents[index], correction));
    }

    return normals;
}

} // namespace

MeshData make_torus_knot(const TorusKnotParameters& parameters) {
    if (parameters.path_segments < 3U || parameters.tube_segments < 3U) {
        throw std::invalid_argument{"Torus-knot segment counts must be at least three."};
    }
    if (parameters.winding_p == 0U || parameters.winding_q == 0U ||
        std::gcd(parameters.winding_p, parameters.winding_q) != 1U) {
        throw std::invalid_argument{"Torus-knot windings must be positive and coprime."};
    }
    if (!is_finite_positive(parameters.major_radius) ||
        !is_finite_positive(parameters.knot_radius) ||
        !is_finite_positive(parameters.tube_radius) ||
        parameters.major_radius <= parameters.knot_radius + parameters.tube_radius) {
        throw std::invalid_argument{
            "Torus-knot radii must be finite, positive, and leave an open center."};
    }

    const std::uint64_t vertex_count =
        static_cast<std::uint64_t>(parameters.path_segments) * parameters.tube_segments;
    validate_vertex_count(vertex_count);

    std::vector<glm::vec3> centers(parameters.path_segments);
    std::vector<glm::vec3> tangents(parameters.path_segments);
    for (std::uint32_t path_index = 0; path_index < parameters.path_segments; ++path_index) {
        const float parameter = full_turn * static_cast<float>(path_index) /
                                static_cast<float>(parameters.path_segments);
        centers[path_index] = knot_position(parameters, parameter);
        tangents[path_index] = knot_tangent(parameters, parameter);
    }
    const std::vector<glm::vec3> normals = make_rotation_minimizing_normals(tangents);

    MeshData mesh;
    mesh.vertices.reserve(static_cast<std::size_t>(vertex_count));

    for (std::uint32_t path_index = 0; path_index < parameters.path_segments; ++path_index) {
        const glm::vec3 binormal =
            glm::normalize(glm::cross(normals[path_index], tangents[path_index]));

        for (std::uint32_t tube_index = 0; tube_index < parameters.tube_segments; ++tube_index) {
            const float tube_angle = full_turn * static_cast<float>(tube_index) /
                                     static_cast<float>(parameters.tube_segments);
            const glm::vec3 surface_normal =
                normals[path_index] * std::cos(tube_angle) + binormal * std::sin(tube_angle);
            mesh.vertices.push_back(MeshVertex{
                .position = centers[path_index] + parameters.tube_radius * surface_normal,
                .normal = surface_normal,
            });
        }
    }

    append_wrapped_grid_indices(mesh, parameters.path_segments, parameters.tube_segments);
    return mesh;
}

} // namespace qws
