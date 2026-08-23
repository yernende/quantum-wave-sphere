#pragma once

#include <cstdint>
#include <glm/vec3.hpp>
#include <vector>

namespace qws {

struct MeshData {
    std::vector<glm::vec3> positions;
    std::vector<std::uint32_t> indices;
};

[[nodiscard]]
MeshData make_icosphere(std::uint32_t subdivisions);

} // namespace qws