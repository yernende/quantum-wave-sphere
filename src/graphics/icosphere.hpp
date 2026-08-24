#pragma once

#include "graphics/mesh_data.hpp"

#include <cstdint>

namespace qws {

[[nodiscard]]
MeshData make_icosphere(std::uint32_t subdivisions);

} // namespace qws
