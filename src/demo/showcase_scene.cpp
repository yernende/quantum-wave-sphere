#include "demo/showcase_scene.hpp"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

namespace qws {

void configure_showcase_render_state() noexcept {
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
}

void clear_showcase_frame(FramebufferSize framebuffer_size) noexcept {
    glViewport(0, 0, framebuffer_size.width, framebuffer_size.height);
    glClearColor(0.025F, 0.035F, 0.055F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

glm::mat4 make_showcase_view_matrix() {
    return glm::lookAt(glm::vec3{1.15F, 0.85F, 2.65F}, glm::vec3{0.0F},
                       glm::vec3{0.0F, 1.0F, 0.0F});
}

glm::mat4 make_showcase_projection_matrix(FramebufferSize framebuffer_size) {
    return glm::perspective(glm::radians(45.0F),
                            static_cast<float>(framebuffer_size.width) /
                                static_cast<float>(framebuffer_size.height),
                            0.1F, 100.0F);
}

} // namespace qws
