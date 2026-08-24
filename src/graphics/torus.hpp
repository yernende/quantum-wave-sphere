#pragma once

#include "graphics/mesh_data.hpp"

#include <cstdint>

namespace qws {

struct TorusParameters {
    std::uint32_t major_segments;
    std::uint32_t minor_segments;
    float major_radius;
    float minor_radius;
};

[[nodiscard]] MeshData make_torus(const TorusParameters& parameters);

} // namespace qws
