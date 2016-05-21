project (halley-entity)

include_directories("include/halley/entity" "../utils/include")

set(SOURCES
        "src/gl_utils.cpp"
        "src/opengl_plugin.cpp"
        "src/painter_opengl.cpp"
        "src/render_target_opengl.cpp"
        "src/shader_opengl.cpp"
        "src/texture_opengl.cpp"
        "src/video_opengl.cpp"
        )

add_library (halley-entity ${SOURCES})