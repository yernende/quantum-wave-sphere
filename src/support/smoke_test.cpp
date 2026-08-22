#include "support/smoke_test.hpp"

#include <glad/gl.h>
#include <iostream>
#include <stdexcept>
#include <string>

namespace qws {
namespace {

constexpr std::size_t smoke_frame_limit = 3;

} // namespace

SmokeTest::SmokeTest(RunMode run_mode) noexcept : run_mode_{run_mode} {}

bool SmokeTest::enabled() const noexcept {
    return run_mode_ == RunMode::smoke_test;
}

void SmokeTest::frame_rendered() {
    if (!enabled() || complete()) {
        return;
    }

    glFinish();
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        throw std::runtime_error{"OpenGL smoke frame failed with error code " +
                                 std::to_string(error) + '.'};
    }

    ++rendered_frames_;
    if (complete()) {
        std::cout << "Smoke test rendered " << rendered_frames_ << " frames successfully.\n";
    }
}

bool SmokeTest::complete() const noexcept {
    return enabled() && rendered_frames_ >= smoke_frame_limit;
}

void SmokeTest::verify_complete() const {
    if (enabled() && !complete()) {
        throw std::runtime_error{"Smoke test ended before rendering three complete frames."};
    }
}

} // namespace qws
