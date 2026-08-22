include_guard(GLOBAL)

option(QWS_WARNINGS_AS_ERRORS "Treat first-party compiler warnings as errors" OFF)
option(QWS_ENABLE_SANITIZERS "Enable supported address and undefined-behavior sanitizers" OFF)
option(QWS_ENABLE_CLANG_TIDY "Run clang-tidy while compiling first-party C++ targets" OFF)

if(QWS_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    include(CheckCXXSourceCompiles)
    include(CMakePushCheckState)

    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_FLAGS "/fsanitize=address")
    set(CMAKE_REQUIRED_LINK_OPTIONS "/INCREMENTAL:NO")
    check_cxx_source_compiles(
        "int main() { return 0; }"
        QWS_MSVC_ASAN_RUNTIME_AVAILABLE
    )
    cmake_pop_check_state()

    if(NOT QWS_MSVC_ASAN_RUNTIME_AVAILABLE)
        message(FATAL_ERROR
            "MSVC AddressSanitizer was requested, but its runtime libraries are unavailable. "
            "Install the optional MSVC AddressSanitizer component or use a non-sanitized preset."
        )
    endif()
endif()

if(QWS_ENABLE_CLANG_TIDY)
    find_program(
        QWS_CLANG_TIDY_EXECUTABLE
        NAMES clang-tidy clang-tidy-20 clang-tidy-19 clang-tidy-18
        DOC "Path to clang-tidy"
    )
    if(NOT QWS_CLANG_TIDY_EXECUTABLE)
        message(FATAL_ERROR "QWS_ENABLE_CLANG_TIDY is ON, but clang-tidy was not found.")
    endif()
endif()

function(qws_configure_target target_name)
    target_compile_features(${target_name} PRIVATE cxx_std_23)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(
            ${target_name}
            PRIVATE
                /W4
                /permissive-
                /utf-8
                /Zc:__cplusplus
                /Zc:preprocessor
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
        target_compile_options(
            ${target_name}
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wsign-conversion
                -Wshadow
                -Wformat=2
                -Wundef
                -Wnon-virtual-dtor
                -Wold-style-cast
                -Woverloaded-virtual
        )
    else()
        message(WARNING "No strict warning set is defined for ${CMAKE_CXX_COMPILER_ID}.")
    endif()

    if(QWS_WARNINGS_AS_ERRORS)
        set_property(TARGET ${target_name} PROPERTY COMPILE_WARNING_AS_ERROR ON)
    endif()

    if(QWS_ENABLE_SANITIZERS)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            # vcpkg libraries are not built with MSVC's STL ASan annotations. Keep the
            # annotation ABI consistent while retaining AddressSanitizer instrumentation.
            target_compile_definitions(
                ${target_name}
                PRIVATE
                    _DISABLE_STRING_ANNOTATION
                    _DISABLE_VECTOR_ANNOTATION
            )
            target_compile_options(${target_name} PRIVATE /fsanitize=address)
            target_link_options(${target_name} PRIVATE /INCREMENTAL:NO)
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
            target_compile_options(
                ${target_name}
                PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer
            )
            target_link_options(
                ${target_name}
                PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer
            )
        else()
            message(FATAL_ERROR
                "Sanitizers are not configured for ${CMAKE_CXX_COMPILER_ID}."
            )
        endif()
    endif()

    if(QWS_ENABLE_CLANG_TIDY)
        set(qws_clang_tidy_command
            "${QWS_CLANG_TIDY_EXECUTABLE}"
            "--config-file=${PROJECT_SOURCE_DIR}/.clang-tidy"
        )
        if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            # clang-tidy needs the exception model explicitly when replaying an MSVC command.
            list(APPEND qws_clang_tidy_command "--extra-arg=/EHsc")
        endif()

        set_property(
            TARGET ${target_name}
            PROPERTY CXX_CLANG_TIDY "${qws_clang_tidy_command}"
        )
    endif()
endfunction()
