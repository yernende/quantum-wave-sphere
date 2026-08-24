cmake_minimum_required(VERSION 3.25)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(FATAL_ERROR
        "macOS is unsupported: quantum-wave-sphere requires OpenGL 4.6 Core and GLSL 460."
    )
endif()

project(
    quantum_wave_sphere
    VERSION 0.1.0
    DESCRIPTION "OpenGL showcase for procedural wave-surface visualizations"
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
        src/capture/animation_capture.cpp
        src/capture/animation_capture.hpp
        src/demo/showcase_scene.cpp
        src/demo/showcase_scene.hpp
        src/graphics/shader_program.cpp
        src/graphics/shader_program.hpp
        src/graphics/geometry_catalog.cpp
        src/graphics/geometry_catalog.hpp
        src/graphics/icosphere.cpp
        src/graphics/icosphere.hpp
        src/graphics/mesh_data.hpp
        src/graphics/superellipsoid.cpp
        src/graphics/superellipsoid.hpp
        src/graphics/suzanne.cpp
        src/graphics/suzanne_control_mesh.hpp
        src/graphics/suzanne.hpp
        src/graphics/torus.cpp
        src/graphics/torus.hpp
        src/graphics/torus_knot.cpp
        src/graphics/torus_knot.hpp
        src/support/app_options.hpp
        src/support/opengl_diagnostics.cpp
        src/support/opengl_diagnostics.hpp
        src/support/smoke_test.cpp
        src/support/smoke_test.hpp
        src/ui/imgui_session.cpp
        src/ui/imgui_session.hpp
        src/demo/wave_mesh.hpp
        src/demo/wave_mesh.cpp
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

    add_executable(quantum_wave_sphere_tests)
    target_sources(
        quantum_wave_sphere_tests
        PRIVATE
            src/graphics/geometry_catalog.cpp
            src/graphics/geometry_catalog.hpp
            src/graphics/icosphere.hpp
            src/graphics/icosphere.cpp
            src/graphics/mesh_data.hpp
            src/graphics/superellipsoid.cpp
            src/graphics/superellipsoid.hpp
            src/graphics/suzanne.cpp
            src/graphics/suzanne_control_mesh.hpp
            src/graphics/suzanne.hpp
            src/graphics/torus.cpp
            src/graphics/torus.hpp
            src/graphics/torus_knot.cpp
            src/graphics/torus_knot.hpp
            tests/app_options_test.cpp
            tests/icosphere_test.cpp
            tests/mesh_test_support.hpp
            tests/superellipsoid_test.cpp
            tests/suzanne_test.cpp
            tests/torus_test.cpp
            tests/torus_knot_test.cpp
    )
    set_target_properties(quantum_wave_sphere_tests PROPERTIES CXX_EXTENSIONS OFF)
    target_include_directories(
        quantum_wave_sphere_tests
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    target_link_libraries(
        quantum_wave_sphere_tests
        PRIVATE
            Catch2::Catch2WithMain
            glm::glm
    )
    qws_configure_target(quantum_wave_sphere_tests)

    include(Catch)
    catch_discover_tests(quantum_wave_sphere_tests)
endif()
