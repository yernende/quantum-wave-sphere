#include "graphics/suzanne.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/geometric.hpp>

TEST_CASE("embedded Suzanne control cage contains valid outward-facing geometry") {
    constexpr std::size_t expected_vertex_count = 507U;
    constexpr std::size_t expected_triangle_count = 968U;
    constexpr float normal_epsilon = 0.0001F;
    constexpr float position_epsilon = 0.0001F;
    constexpr float triangle_epsilon = 0.00000001F;

    const qws::MeshData mesh = qws::make_suzanne();
    REQUIRE(mesh.vertices.size() == expected_vertex_count);
    REQUIRE(mesh.indices.size() == expected_triangle_count * 3U);

    bool vertices_are_finite = true;
    bool normals_are_unit_length = true;
    float maximum_position_radius = 0.0F;
    for (const qws::MeshVertex& vertex : mesh.vertices) {
        vertices_are_finite = vertices_are_finite && std::isfinite(vertex.position.x) &&
                              std::isfinite(vertex.position.y) &&
                              std::isfinite(vertex.position.z) && std::isfinite(vertex.normal.x) &&
                              std::isfinite(vertex.normal.y) && std::isfinite(vertex.normal.z);
        normals_are_unit_length =
            normals_are_unit_length && std::abs(glm::length(vertex.normal) - 1.0F) < normal_epsilon;
        maximum_position_radius = std::max(maximum_position_radius, glm::length(vertex.position));
    }

    bool indices_are_in_range = true;
    bool triangles_are_nondegenerate = true;
    float signed_volume_times_six = 0.0F;
    for (std::size_t offset = 0; offset < mesh.indices.size(); offset += 3U) {
        const std::uint32_t a_index = mesh.indices[offset];
        const std::uint32_t b_index = mesh.indices[offset + 1U];
        const std::uint32_t c_index = mesh.indices[offset + 2U];
        indices_are_in_range = indices_are_in_range && a_index < mesh.vertices.size() &&
                               b_index < mesh.vertices.size() && c_index < mesh.vertices.size();
        if (!indices_are_in_range) {
            break;
        }

        const qws::MeshVertex& a = mesh.vertices[a_index];
        const qws::MeshVertex& b = mesh.vertices[b_index];
        const qws::MeshVertex& c = mesh.vertices[c_index];
        const glm::vec3 face_normal = glm::cross(b.position - a.position, c.position - a.position);
        triangles_are_nondegenerate =
            triangles_are_nondegenerate && glm::length(face_normal) > triangle_epsilon;
        signed_volume_times_six += glm::dot(a.position, glm::cross(b.position, c.position));
    }

    CAPTURE(maximum_position_radius, signed_volume_times_six);
    CHECK(vertices_are_finite);
    CHECK(normals_are_unit_length);
    CHECK(maximum_position_radius <= 1.0F + position_epsilon);
    CHECK(indices_are_in_range);
    CHECK(triangles_are_nondegenerate);
    CHECK(signed_volume_times_six > 0.0F);
}
