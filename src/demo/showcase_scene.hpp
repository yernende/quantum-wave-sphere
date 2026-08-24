#pragma once

#include <glm/mat4x4.hpp>

namespace qws {

struct FramebufferSize {
    int width;
    int height;
};

void configure_showcase_render_state() noexcept;
void clear_showcase_frame(FramebufferSize framebuffer_size) noexcept;

[[nodiscard]] glm::mat4 make_showcase_view_matrix();
[[nodiscard]] glm::mat4 make_showcase_projection_matrix(FramebufferSize framebuffer_size);

} // namespace qws
