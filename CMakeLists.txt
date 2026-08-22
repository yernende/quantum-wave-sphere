cmake_minimum_required(VERSION 3.25)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(FATAL_ERROR
        "macOS is unsupported: quantum-wave-sphere requires OpenGL 4.6 Core and GLSL 460."
    )
endif()

project(
    quantum_wave_sphere
    VERSION 0.1.0
    DESCRIPTION "OpenGL bootstrap for a procedural spherical-wave visualization"
    LANGUAGES C CXX
)

if(APPLE)
    message(FATAL_ERROR
        "macOS is unsupported: quantum-wave-sphere requires OpenGL 4.6 Core and GLSL 460."
    )
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(ProjectOptions)
include(CTest)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_subdirectory(third_party/glad)

add_executable(quantum_wave_sphere)
target_sources(
    quantum_wave_sphere
    PRIVATE
        src/main.cpp
        src/bootstrap/glfw_context.cpp
        src/bootstrap/glfw_context.hpp
        src/demo/triangle_demo.cpp
        src/demo/triangle_demo.hpp
        src/graphics/shader_program.cpp
        src/graphics/shader_program.hpp
        src/support/app_options.hpp
        src/support/opengl_diagnostics.cpp
        src/support/opengl_diagnostics.hpp
        src/support/smoke_test.cpp
        src/support/smoke_test.hpp
        src/ui/imgui_session.cpp
        src/ui/imgui_session.hpp
)
set_target_properties(
    quantum_wave_sphere
    PROPERTIES
        CXX_EXTENSIONS OFF
        OUTPUT_NAME quantum-wave-sphere
)
target_include_directories(quantum_wave_sphere PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(quantum_wave_sphere PRIVATE GLFW_INCLUDE_NONE)
target_link_libraries(
    quantum_wave_sphere
    PRIVATE
        glad2
        glfw
        glm::glm
        imgui::imgui
)
qws_configure_target(quantum_wave_sphere)

if(BUILD_TESTING)
    find_package(Catch2 3 CONFIG REQUIRED)

    add_executable(quantum_wave_sphere_tests tests/app_options_test.cpp)
    set_target_properties(quantum_wave_sphere_tests PROPERTIES CXX_EXTENSIONS OFF)
    target_include_directories(
        quantum_wave_sphere_tests
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    target_link_libraries(quantum_wave_sphere_tests PRIVATE Catch2::Catch2WithMain)
    qws_configure_target(quantum_wave_sphere_tests)

    include(Catch)
    catch_discover_tests(quantum_wave_sphere_tests)
endif()
