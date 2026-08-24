#pragma once

#include "graphics/mesh_data.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/geometric.hpp>
#include <unordered_map>

namespace qws::test_support {

inline constexpr float normal_epsilon = 0.0001F;
inline constexpr float triangle_epsilon = 0.00000001F;

inline void check_mesh_invariants(const MeshData& mesh) {
    REQUIRE_FALSE(mesh.vertices.empty());
    REQUIRE_FALSE(mesh.indices.empty());
    REQUIRE(mesh.indices.size() % 3U == 0U);

    for (const MeshVertex& vertex : mesh.vertices) {
        CHECK(std::isfinite(vertex.position.x));
        CHECK(std::isfinite(vertex.position.y));
        CHECK(std::isfinite(vertex.position.z));
        CHECK(std::isfinite(vertex.normal.x));
        CHECK(std::isfinite(vertex.normal.y));
        CHECK(std::isfinite(vertex.normal.z));
        CHECK(std::abs(glm::length(vertex.normal) - 1.0F) < normal_epsilon);
    }

    for (const std::uint32_t index : mesh.indices) {
        REQUIRE(index < mesh.vertices.size());
    }

    for (std::size_t offset = 0; offset < mesh.indices.size(); offset += 3U) {
        const MeshVertex& a = mesh.vertices[mesh.indices[offset]];
        const MeshVertex& b = mesh.vertices[mesh.indices[offset + 1U]];
        const MeshVertex& c = mesh.vertices[mesh.indices[offset + 2U]];
        const glm::vec3 face_normal = glm::cross(b.position - a.position, c.position - a.position);
        const glm::vec3 average_normal = a.normal + b.normal + c.normal;

        CHECK(glm::length(face_normal) > triangle_epsilon);
        CHECK(glm::dot(face_normal, average_normal) > 0.0F);
    }
}

[[nodiscard]] constexpr std::uint64_t make_edge_key(std::uint32_t first,
                                                    std::uint32_t second) noexcept {
    const std::uint32_t low = first < second ? first : second;
    const std::uint32_t high = first < second ? second : first;
    return (static_cast<std::uint64_t>(low) << 32U) | high;
}

inline void check_closed_index_topology(const MeshData& mesh) {
    std::unordered_map<std::uint64_t, std::size_t> edge_counts;
    edge_counts.reserve(mesh.indices.size());

    for (std::size_t offset = 0; offset < mesh.indices.size(); offset += 3U) {
        const std::uint32_t a = mesh.indices[offset];
        const std::uint32_t b = mesh.indices[offset + 1U];
        const std::uint32_t c = mesh.indices[offset + 2U];
        ++edge_counts[make_edge_key(a, b)];
        ++edge_counts[make_edge_key(b, c)];
        ++edge_counts[make_edge_key(c, a)];
    }

    for (const auto& [edge, count] : edge_counts) {
        CAPTURE(edge);
        CHECK(count == 2U);
    }
}

inline void check_deterministic(const MeshData& first, const MeshData& second) {
    REQUIRE(first.vertices.size() == second.vertices.size());
    REQUIRE(first.indices == second.indices);

    for (std::size_t index = 0; index < first.vertices.size(); ++index) {
        CHECK(first.vertices[index].position.x == second.vertices[index].position.x);
        CHECK(first.vertices[index].position.y == second.vertices[index].position.y);
        CHECK(first.vertices[index].position.z == second.vertices[index].position.z);
        CHECK(first.vertices[index].normal.x == second.vertices[index].normal.x);
        CHECK(first.vertices[index].normal.y == second.vertices[index].normal.y);
        CHECK(first.vertices[index].normal.z == second.vertices[index].normal.z);
    }
}

} // namespace qws::test_support
