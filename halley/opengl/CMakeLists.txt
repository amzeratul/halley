project (halley-opengl)

include_directories(${SDL2_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} "include/halley/opengl" "../utils/include" "../core/include")

set(SOURCES
        "src/gl_core_3_3.c"
        "src/gl_utils.cpp"
        "src/opengl_plugin.cpp"
        "src/painter_opengl.cpp"
        "src/render_target_opengl.cpp"
        "src/shader_opengl.cpp"
        "src/texture_opengl.cpp"
        "src/video_opengl.cpp"
        )

set(HEADERS
        "include/halley/opengl/gl_core_3_3.h"
        "include/halley/opengl/gl_utils.h"
        "include/halley/opengl/halley_gl.h"
        "include/halley/opengl/painter_opengl.h"
        "include/halley/opengl/prec.h"
        "include/halley/opengl/render_target_opengl.h"
        "include/halley/opengl/shader_opengl.h"
        "include/halley/opengl/texture_opengl.h"
        "include/halley/opengl/video_opengl.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-opengl ${SOURCES} ${HEADERS})
target_link_libraries(halley-opengl halley-core ${OPENGL_LIBRARIES})