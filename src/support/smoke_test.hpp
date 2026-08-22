#pragma once

#include "support/app_options.hpp"

#include <cstddef>

namespace qws {

class SmokeTest final {
  public:
    explicit SmokeTest(RunMode run_mode) noexcept;

    [[nodiscard]] bool enabled() const noexcept;
    void frame_rendered();
    [[nodiscard]] bool complete() const noexcept;
    void verify_complete() const;

  private:
    RunMode run_mode_;
    std::size_t rendered_frames_{0};
};

} // namespace qws
