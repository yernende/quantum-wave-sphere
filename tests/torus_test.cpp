#include "graphics/torus.hpp"
#include "mesh_test_support.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <stdexcept>

TEST_CASE("torus generation produces a closed outward-facing grid") {
    constexpr qws::TorusParameters parameters{
        .major_segments = 8U,
        .minor_segments = 5U,
        .major_radius = 0.7F,
        .minor_radius = 0.3F,
    };
    const qws::MeshData mesh = qws::make_torus(parameters);

    CHECK(mesh.vertices.size() == parameters.major_segments * parameters.minor_segments);
    CHECK(mesh.indices.size() ==
          static_cast<std::size_t>(parameters.major_segments) * parameters.minor_segments * 6U);
    qws::test_support::check_mesh_invariants(mesh);
    qws::test_support::check_closed_index_topology(mesh);
    qws::test_support::check_deterministic(mesh, qws::make_torus(parameters));
}

TEST_CASE("torus generation rejects invalid parameters") {
    CHECK_THROWS_AS(qws::make_torus(qws::TorusParameters{
                        .major_segments = 2U,
                        .minor_segments = 8U,
                        .major_radius = 0.7F,
                        .minor_radius = 0.3F,
                    }),
                    std::invalid_argument);
    CHECK_THROWS_AS(qws::make_torus(qws::TorusParameters{
                        .major_segments = 8U,
                        .minor_segments = 8U,
                        .major_radius = 0.3F,
                        .minor_radius = 0.3F,
                    }),
                    std::invalid_argument);
}
