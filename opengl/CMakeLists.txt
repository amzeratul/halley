project (halley-opengl)

find_Package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
include_directories(${SDL2_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIR} "include/halley/opengl" "../utils/include" "../core/include")

set(SOURCES
        "src/gl_utils.cpp"
        "src/opengl_plugin.cpp"
        "src/painter_opengl.cpp"
        "src/render_target_opengl.cpp"
        "src/shader_opengl.cpp"
        "src/texture_opengl.cpp"
        "src/video_opengl.cpp"
        )

set(HEADERS
        "include/halley/opengl/gl_utils.h"
        "include/halley/opengl/halley_gl.h"
        "include/halley/opengl/painter_opengl.h"
        "include/halley/opengl/prec.h"
        "include/halley/opengl/render_target_opengl.h"
        "include/halley/opengl/shader_opengl.h"
        "include/halley/opengl/texture_opengl.h"
        "include/halley/opengl/video_opengl.h"
        )

add_library (halley-opengl ${SOURCES} ${HEADERS})
target_link_libraries(halley-opengl halley-core ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})