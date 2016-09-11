project (halley-opengl)

include_directories(${Boost_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} "include/halley/opengl" "../utils/include" "../core/include")

set(SOURCES
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
        "include/halley/opengl/gl_utils.h"
        "include/halley/opengl/halley_gl.h"
        "include/halley/opengl/loader_thread_opengl.h"
        "include/halley/opengl/painter_opengl.h"
        "include/halley/opengl/prec.h"
        "include/halley/opengl/render_target_opengl.h"
        "include/halley/opengl/shader_opengl.h"
        "include/halley/opengl/texture_opengl.h"
        "include/halley/opengl/video_opengl.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

set(WITH_OPENGL 1)

if (WITH_OPENGL)
	add_definitions(-DWITH_OPENGL)
	set(SOURCES ${SOURCES} "src/gl_core_3_3.c")
	set(HEADERS ${HEADERS} "include/halley/opengl/gl_core_3_3.h")
elseif(WITH_OPENGL_ES2)
	add_definitions(-DWITH_OPENGL_ES2)
endif()

add_library (halley-opengl ${SOURCES} ${HEADERS})
target_link_libraries(halley-opengl halley-core ${OPENGL_LIBRARIES})