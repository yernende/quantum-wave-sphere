#include "graphics/shader_program.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

namespace qws {
namespace {

std::string shader_log(GLuint shader) {
    GLint log_length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length <= 1) {
        return {};
    }

    std::string log(static_cast<std::size_t>(log_length), '\0');
    GLsizei written = 0;
    glGetShaderInfoLog(shader, log_length, &written, log.data());
    log.resize(static_cast<std::size_t>(written));
    return log;
}

std::string program_log(GLuint program) {
    GLint log_length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length <= 1) {
        return {};
    }

    std::string log(static_cast<std::size_t>(log_length), '\0');
    GLsizei written = 0;
    glGetProgramInfoLog(program, log_length, &written, log.data());
    log.resize(static_cast<std::size_t>(written));
    return log;
}

GLuint compile_shader(GLenum type, std::string_view source) {
    const GLuint shader = glCreateShader(type);
    if (shader == 0) {
        throw std::runtime_error{"OpenGL failed to allocate a shader object."};
    }

    const GLchar* source_data = source.data();
    // Keep the OpenGL API width visible at this narrowing boundary.
    // NOLINTNEXTLINE(modernize-use-auto)
    const GLint source_length = static_cast<GLint>(source.size());
    glShaderSource(shader, 1, &source_data, &source_length);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        const std::string log = shader_log(shader);
        glDeleteShader(shader);
        throw std::runtime_error{"Shader compilation failed:\n" + log};
    }

    return shader;
}

} // namespace

// The names carry fixed shader-stage roles; strong wrapper types would add noise here.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
ShaderProgram::ShaderProgram(std::string_view vertex_source, std::string_view fragment_source) {
    GLuint vertex_shader = 0;
    GLuint fragment_shader = 0;

    try {
        vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_source);
        fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source);

        program_ = glCreateProgram();
        if (program_ == 0) {
            throw std::runtime_error{"OpenGL failed to allocate a program object."};
        }

        glAttachShader(program_, vertex_shader);
        glAttachShader(program_, fragment_shader);
        glLinkProgram(program_);

        GLint linked = GL_FALSE;
        glGetProgramiv(program_, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE) {
            throw std::runtime_error{"Shader link failed:\n" + program_log(program_)};
        }
    } catch (...) {
        if (program_ != 0) {
            glDeleteProgram(program_);
            program_ = 0;
        }
        if (fragment_shader != 0) {
            glDeleteShader(fragment_shader);
        }
        if (vertex_shader != 0) {
            glDeleteShader(vertex_shader);
        }
        throw;
    }

    glDeleteShader(fragment_shader);
    glDeleteShader(vertex_shader);
}

ShaderProgram::~ShaderProgram() {
    if (program_ != 0) {
        glDeleteProgram(program_);
    }
}

GLuint ShaderProgram::id() const noexcept {
    return program_;
}

} // namespace qws
