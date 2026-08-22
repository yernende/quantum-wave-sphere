#pragma once

#include <glad/gl.h>
#include <string_view>

namespace qws {

// A current OpenGL context must outlive every shader program.
class ShaderProgram final {
  public:
    // The names carry fixed shader-stage roles; strong wrapper types would add noise here.
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    ShaderProgram(std::string_view vertex_source, std::string_view fragment_source);
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&&) = delete;
    ShaderProgram& operator=(ShaderProgram&&) = delete;

    [[nodiscard]] GLuint id() const noexcept;

  private:
    GLuint program_{0};
};

} // namespace qws
