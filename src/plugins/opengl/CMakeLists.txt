project (halley-opengl)

include_directories(${Boost_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} "../../engine/utils/include" "../../engine/core/include")

set(SOURCES
        "src/constant_buffer_opengl.cpp"
        "src/gl_buffer.cpp"
        "src/gl_utils.cpp"
        "src/loader_thread_opengl.cpp"
        "src/opengl_plugin.cpp"
        "src/painter_opengl.cpp"
        "src/render_target_opengl.cpp"
        "src/shader_opengl.cpp"
        "src/texture_opengl.cpp"
        "src/video_opengl.cpp"
        )

set(HEADERS
        "src/constant_buffer_opengl.h"
        "src/gl_buffer.h"
        "src/gl_utils.h"
        "src/halley_gl.h"
        "src/loader_thread_opengl.h"
        "src/painter_opengl.h"
        "src/prec.h"
        "src/render_target_opengl.h"
        "src/shader_opengl.h"
        "src/texture_opengl.h"
        "src/video_opengl.h"
        )

if (USE_OPENGL)
	set(SOURCES ${SOURCES} "src/gl_core_3_3.c")
	set(HEADERS ${HEADERS} "src/gl_core_3_3.h")
endif()

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-opengl ${SOURCES} ${HEADERS})
target_link_libraries(halley-opengl halley-core halley-utils ${OPENGL_LIBRARIES})
