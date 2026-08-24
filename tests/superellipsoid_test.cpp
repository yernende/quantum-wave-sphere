#include "graphics/superellipsoid.hpp"
#include "mesh_test_support.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <glm/geometric.hpp>
#include <limits>
#include <stdexcept>

TEST_CASE("superellipsoid generation produces six consistently wound face grids") {
    constexpr qws::SuperellipsoidParameters parameters{
        .face_segments = 4U,
        .exponent = 4.0F,
        .bounding_radius = 1.0F,
    };
    const qws::MeshData mesh = qws::make_superellipsoid(parameters);
    const std::size_t vertices_per_edge = parameters.face_segments + 1U;

    CHECK(mesh.vertices.size() == 6U * vertices_per_edge * vertices_per_edge);
    CHECK(mesh.indices.size() ==
          static_cast<std::size_t>(parameters.face_segments) * parameters.face_segments * 36U);
    qws::test_support::check_mesh_invariants(mesh);
    qws::test_support::check_deterministic(mesh, qws::make_superellipsoid(parameters));

    for (const qws::MeshVertex& vertex : mesh.vertices) {
        CHECK(glm::length(vertex.position) <=
              parameters.bounding_radius + qws::test_support::normal_epsilon);
    }
}

TEST_CASE("superellipsoid generation rejects invalid parameters") {
    CHECK_THROWS_AS(qws::make_superellipsoid(qws::SuperellipsoidParameters{
                        .face_segments = 4U,
                        .exponent = std::numeric_limits<float>::quiet_NaN(),
                        .bounding_radius = 1.0F,
                    }),
                    std::invalid_argument);
}
