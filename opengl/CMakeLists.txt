project (halley-opengl)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules/")

find_Package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR} "../utils/src" "../core/src")

set(SOURCES
        "src/gl_utils.cpp"
        "src/opengl_plugin.cpp"
        "src/painter_opengl.cpp"
        "src/render_target_opengl.cpp"
        "src/shader_opengl.cpp"
        "src/texture_opengl.cpp"
        "src/video_opengl.cpp"
        )

add_library (halley-opengl ${SOURCES})