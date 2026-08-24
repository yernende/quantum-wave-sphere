#include "graphics/geometry_catalog.hpp"

#include "graphics/icosphere.hpp"
#include "graphics/superellipsoid.hpp"
#include "graphics/suzanne.hpp"
#include "graphics/torus.hpp"
#include "graphics/torus_knot.hpp"

#include <stdexcept>

namespace qws {

MeshData make_showcase_geometry(GeometryKind kind) {
    switch (kind) {
    case GeometryKind::icosphere:
        return make_icosphere(7U);
    case GeometryKind::torus:
        return make_torus(TorusParameters{
            .major_segments = 256U,
            .minor_segments = 64U,
            .major_radius = 0.70F,
            .minor_radius = 0.30F,
        });
    case GeometryKind::superellipsoid:
        return make_superellipsoid(SuperellipsoidParameters{
            .face_segments = 96U,
            .exponent = 4.0F,
            .bounding_radius = 1.0F,
        });
    case GeometryKind::trefoil_knot:
        return make_torus_knot(TorusKnotParameters{
            .path_segments = 384U,
            .tube_segments = 48U,
            .winding_p = 2U,
            .winding_q = 3U,
            .major_radius = 0.65F,
            .knot_radius = 0.25F,
            .tube_radius = 0.10F,
        });
    case GeometryKind::suzanne:
        return make_suzanne();
    }

    throw std::invalid_argument{"Unknown procedural geometry kind."};
}

} // namespace qws
