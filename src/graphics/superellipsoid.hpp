#pragma once

#include "graphics/mesh_data.hpp"

#include <cstdint>

namespace qws {

struct SuperellipsoidParameters {
    std::uint32_t face_segments;
    float exponent;
    float bounding_radius;
};

[[nodiscard]] MeshData make_superellipsoid(const SuperellipsoidParameters& parameters);

} // namespace qws
