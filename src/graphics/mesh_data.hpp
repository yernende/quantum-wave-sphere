#pragma once

#include <cstdint>
#include <glm/vec3.hpp>
#include <vector>

namespace qws {

struct MeshVertex {
    glm::vec3 position;
    glm::vec3 normal;
};

struct MeshData {
    std::vector<MeshVertex> vertices;
    std::vector<std::uint32_t> indices;
};

} // namespace qws
