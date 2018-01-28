project (halley-dx11)

include_directories(${Boost_INCLUDE_DIR} "../../engine/utils/include" "../../engine/core/include")

set(SOURCES
        "src/dx11_blend.cpp"
        "src/dx11_buffer.cpp"
        "src/dx11_loader.cpp"
        "src/dx11_material_constant_buffer.cpp"
        "src/dx11_plugin.cpp"
        "src/dx11_painter.cpp"
        "src/dx11_rasterizer.cpp"
        "src/dx11_render_target.cpp"
        "src/dx11_shader.cpp"
        "src/dx11_texture.cpp"
        "src/dx11_video.cpp"
        )

set(HEADERS
        "src/dx11_blend.h"
        "src/dx11_buffer.h"
        "src/dx11_loader.h"
        "src/dx11_material_constant_buffer.h"
        "src/dx11_painter.h"
        "src/dx11_rasterizer.h"
        "src/dx11_render_target.h"
        "src/dx11_shader.h"
        "src/dx11_texture.h"
        "src/dx11_video.h"
        "src/halley_dx11.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-dx11 ${SOURCES} ${HEADERS})
target_link_libraries(halley-dx11 halley-core halley-utils)
