#include "graphics/torus_knot.hpp"
#include "mesh_test_support.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <stdexcept>

TEST_CASE("torus-knot generation closes the swept tube without a frame flip") {
    constexpr qws::TorusKnotParameters parameters{
        .path_segments = 24U,
        .tube_segments = 8U,
        .winding_p = 2U,
        .winding_q = 3U,
        .major_radius = 0.65F,
        .knot_radius = 0.25F,
        .tube_radius = 0.10F,
    };
    const qws::MeshData mesh = qws::make_torus_knot(parameters);

    CHECK(mesh.vertices.size() == parameters.path_segments * parameters.tube_segments);
    CHECK(mesh.indices.size() ==
          static_cast<std::size_t>(parameters.path_segments) * parameters.tube_segments * 6U);
    qws::test_support::check_mesh_invariants(mesh);
    qws::test_support::check_closed_index_topology(mesh);
    qws::test_support::check_deterministic(mesh, qws::make_torus_knot(parameters));
}

TEST_CASE("torus-knot generation rejects invalid parameters") {
    CHECK_THROWS_AS(qws::make_torus_knot(qws::TorusKnotParameters{
                        .path_segments = 24U,
                        .tube_segments = 8U,
                        .winding_p = 2U,
                        .winding_q = 4U,
                        .major_radius = 0.65F,
                        .knot_radius = 0.25F,
                        .tube_radius = 0.10F,
                    }),
                    std::invalid_argument);
}
