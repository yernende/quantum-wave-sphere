#pragma once

#include "graphics/mesh_data.hpp"

#include <cstdint>

namespace qws {

struct TorusKnotParameters {
    std::uint32_t path_segments;
    std::uint32_t tube_segments;
    std::uint32_t winding_p;
    std::uint32_t winding_q;
    float major_radius;
    float knot_radius;
    float tube_radius;
};

[[nodiscard]] MeshData make_torus_knot(const TorusKnotParameters& parameters);

} // namespace qws
