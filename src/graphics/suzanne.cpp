#include "graphics/suzanne.hpp"

#include "graphics/suzanne_control_mesh.hpp"

namespace qws {

MeshData make_suzanne() {
    MeshData mesh;
    mesh.vertices.assign(suzanne_control_mesh::vertices.begin(),
                         suzanne_control_mesh::vertices.end());
    mesh.indices.assign(suzanne_control_mesh::indices.begin(), suzanne_control_mesh::indices.end());
    return mesh;
}

static_assert(suzanne_control_mesh::vertices.size() == 507U);
static_assert(suzanne_control_mesh::indices.size() == 968U * 3U);

} // namespace qws
