#pragma once

#include <iosfwd>
#include <string>

namespace qws {

struct FrameCaptureOptions {
    std::string output_file;
    std::string geometry_slug;
    int frames_per_second{24};
    int width{640};
    int height{360};
};

void write_capture_geometry_slugs(std::ostream& output);
void capture_animation(const FrameCaptureOptions& options);

} // namespace qws
