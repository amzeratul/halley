project (halley-metal)

include_directories(${Boost_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${SDL2_INCLUDE_DIR} "../../engine/utils/include" "../../engine/core/include")

set(SOURCES
        "src/metal_plugin.mm"

        "src/metal_buffer.mm"
        "src/metal_loader.mm"
        "src/metal_material_constant_buffer.mm"
        "src/metal_painter.mm"
        "src/metal_render_target.mm"
        "src/metal_shader.mm"
        "src/metal_texture.mm"
        "src/metal_video.mm"
        )

set(HEADERS
        "src/metal_buffer.h"
        "src/metal_loader.h"
        "src/metal_material_constant_buffer.h"
        "src/metal_painter.h"
        "src/metal_render_target.h"
        "src/metal_shader.h"
        "src/metal_texture.h"
        "src/metal_video.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -x objective-c++")

add_library (halley-metal ${SOURCES} ${HEADERS})
set_target_properties(halley-metal PROPERTIES DISABLE_PRECOMPILE_HEADERS ON)
find_library(METAL_LIBRARY Metal)
find_library(QUARTZ_LIBRARY QuartzCore)
target_link_libraries(halley-metal halley-engine "${METAL_LIBRARY}" "${QUARTZ_LIBRARY}")
