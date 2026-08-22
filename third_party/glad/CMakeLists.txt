add_library(glad2 STATIC src/gl.c)
target_compile_features(glad2 PRIVATE c_std_11)
target_include_directories(
    glad2
    SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(glad2 PUBLIC ${CMAKE_DL_LIBS})
