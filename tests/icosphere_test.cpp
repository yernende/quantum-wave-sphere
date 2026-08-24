#include "graphics/icosphere.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/geometric.hpp>

namespace {

struct ExpectedMeshSize {
    std::uint32_t subdivisions;
    std::size_t vertices;
    std::size_t faces;
};

} // namespace

TEST_CASE("icosphere subdivision levels produce the expected mesh sizes") {
    constexpr std::array expected_sizes{
        ExpectedMeshSize{.subdivisions = 0, .vertices = 12, .faces = 20},
        ExpectedMeshSize{.subdivisions = 3, .vertices = 642, .faces = 1280},
        ExpectedMeshSize{.subdivisions = 4, .vertices = 2562, .faces = 5120},
    };

    for (const ExpectedMeshSize& expected : expected_sizes) {
        const qws::MeshData mesh = qws::make_icosphere(expected.subdivisions);

        CAPTURE(expected.subdivisions);
        CHECK(mesh.vertices.size() == expected.vertices);
        CHECK(mesh.indices.size() == expected.faces * 3);
    }
}

TEST_CASE("icosphere positions and indices contain valid data") {
    constexpr float epsilon = 0.00001F;
    const qws::MeshData mesh = qws::make_icosphere(4);

    for (const qws::MeshVertex& vertex : mesh.vertices) {
        CHECK(std::abs(glm::length(vertex.position) - 1.0F) < epsilon);
        CHECK(std::abs(glm::length(vertex.normal) - 1.0F) < epsilon);
    }

    for (const std::uint32_t index : mesh.indices) {
        CHECK(index < mesh.vertices.size());
    }
}
