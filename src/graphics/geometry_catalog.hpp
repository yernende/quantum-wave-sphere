#pragma once

#include "graphics/mesh_data.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace qws {

enum class GeometryKind : std::uint8_t {
    icosphere,
    torus,
    superellipsoid,
    trefoil_knot,
    suzanne,
};

inline constexpr std::array all_geometry_kinds{
    GeometryKind::icosphere,    GeometryKind::torus,   GeometryKind::superellipsoid,
    GeometryKind::trefoil_knot, GeometryKind::suzanne,
};
inline constexpr std::size_t geometry_kind_count = all_geometry_kinds.size();

[[nodiscard]] constexpr std::size_t geometry_index(GeometryKind kind) noexcept {
    return static_cast<std::size_t>(kind);
}

[[nodiscard]] constexpr std::string_view geometry_name(GeometryKind kind) noexcept {
    constexpr std::array names{
        std::string_view{"Icosphere"},      std::string_view{"Torus"},
        std::string_view{"Superellipsoid"}, std::string_view{"Trefoil knot"},
        std::string_view{"Suzanne"},
    };
    return names[geometry_index(kind)];
}

[[nodiscard]] MeshData make_showcase_geometry(GeometryKind kind);
[[nodiscard]] std::array<MeshData, geometry_kind_count> make_showcase_geometries();

} // namespace qws
