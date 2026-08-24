#include "graphics/icosphere.hpp"

#include <algorithm>
#include <cstddef>
#include <glm/geometric.hpp>
#include <numbers>
#include <unordered_map>
#include <utility>
#include <vector>

namespace qws {
namespace {

using MidpointCache = std::unordered_map<std::uint64_t, std::uint32_t>;

MeshVertex make_sphere_vertex(const glm::vec3& position) {
    const glm::vec3 normalized_position = glm::normalize(position);
    return MeshVertex{.position = normalized_position, .normal = normalized_position};
}

MeshData make_base_icosphere() {
    constexpr float phi = std::numbers::phi_v<float>;

    return MeshData{
        .vertices =
            {
                make_sphere_vertex(glm::vec3{-1.0F, phi, 0.0F}),
                make_sphere_vertex(glm::vec3{1.0F, phi, 0.0F}),
                make_sphere_vertex(glm::vec3{-1.0F, -phi, 0.0F}),
                make_sphere_vertex(glm::vec3{1.0F, -phi, 0.0F}),

                make_sphere_vertex(glm::vec3{0.0F, -1.0F, phi}),
                make_sphere_vertex(glm::vec3{0.0F, 1.0F, phi}),
                make_sphere_vertex(glm::vec3{0.0F, -1.0F, -phi}),
                make_sphere_vertex(glm::vec3{0.0F, 1.0F, -phi}),

                make_sphere_vertex(glm::vec3{phi, 0.0F, -1.0F}),
                make_sphere_vertex(glm::vec3{phi, 0.0F, 1.0F}),
                make_sphere_vertex(glm::vec3{-phi, 0.0F, -1.0F}),
                make_sphere_vertex(glm::vec3{-phi, 0.0F, 1.0F}),
            },
        .indices =
            {
                0,  11, 5,

                0,  5,  1,

                0,  1,  7,

                0,  7,  10,

                0,  10, 11,

                1,  5,  9,

                5,  11, 4,

                11, 10, 2,

                10, 7,  6,

                7,  1,  8,

                3,  9,  4,

                3,  4,  2,

                3,  2,  6,

                3,  6,  8,

                3,  8,  9,

                4,  9,  5,

                2,  4,  11,

                6,  2,  10,

                8,  6,  7,

                9,  8,  1,
            },
    };
}

// An edge has no direction, so its endpoint parameters are intentionally interchangeable.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
constexpr std::uint64_t make_edge_key(std::uint32_t first, std::uint32_t second) noexcept {
    const std::uint32_t low = std::min(first, second);
    const std::uint32_t high = std::max(first, second);

    return (static_cast<std::uint64_t>(low) << 32U) | static_cast<std::uint64_t>(high);
}

// An edge has no direction, so its endpoint parameters are intentionally interchangeable.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
std::uint32_t get_or_create_midpoint_index(MeshData& mesh, MidpointCache& cache,
                                           std::uint32_t first, std::uint32_t second) {
    const std::uint64_t key = make_edge_key(first, second);
    if (const auto cached = cache.find(key); cached != cache.end()) {
        return cached->second;
    }

    const glm::vec3 midpoint =
        glm::normalize((mesh.vertices[first].position + mesh.vertices[second].position) * 0.5F);
    const auto index = static_cast<std::uint32_t>(mesh.vertices.size());

    mesh.vertices.push_back(MeshVertex{.position = midpoint, .normal = midpoint});
    cache.emplace(key, index);

    return index;
}

} // namespace

MeshData make_icosphere(std::uint32_t subdivisions) {
    MeshData mesh = make_base_icosphere();

    for (std::uint32_t level = 0; level < subdivisions; ++level) {
        MidpointCache midpoint_cache;
        midpoint_cache.reserve(mesh.indices.size() / 2);

        std::vector<std::uint32_t> subdivided_indices;
        subdivided_indices.reserve(mesh.indices.size() * 4);

        for (std::size_t offset = 0; offset < mesh.indices.size(); offset += 3) {
            const std::uint32_t a_index = mesh.indices[offset];
            const std::uint32_t b_index = mesh.indices[offset + 1];
            const std::uint32_t c_index = mesh.indices[offset + 2];

            const std::uint32_t ab_midpoint_index =
                get_or_create_midpoint_index(mesh, midpoint_cache, a_index, b_index);
            const std::uint32_t bc_midpoint_index =
                get_or_create_midpoint_index(mesh, midpoint_cache, b_index, c_index);
            const std::uint32_t ca_midpoint_index =
                get_or_create_midpoint_index(mesh, midpoint_cache, c_index, a_index);

            subdivided_indices.insert(subdivided_indices.end(), {
                                                                    a_index,
                                                                    ab_midpoint_index,
                                                                    ca_midpoint_index,

                                                                    b_index,
                                                                    bc_midpoint_index,
                                                                    ab_midpoint_index,

                                                                    c_index,
                                                                    ca_midpoint_index,
                                                                    bc_midpoint_index,

                                                                    ab_midpoint_index,
                                                                    bc_midpoint_index,
                                                                    ca_midpoint_index,
                                                                });
        }

        mesh.indices = std::move(subdivided_indices);
    }

    return mesh;
}

} // namespace qws
