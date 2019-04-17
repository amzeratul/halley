project (halley-metal)

include_directories(${Boost_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} "../../engine/utils/include" "../../engine/core/include")

set(SOURCES
        "src/metal_plugin.mm"
        "src/metal_video.mm"
        "src/metal_shader.mm"
        "src/metal_buffer.mm"
        "src/metal_material_constant_buffer.mm"
        "src/metal_loader.mm"
        )

set(HEADERS
        "src/metal_video.h"
        "src/metal_shader.h"
        "src/metal_buffer.h"
        "src/metal_material_constant_buffer.h"
        "src/metal_loader.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -x objective-c++")

add_library (halley-metal ${SOURCES} ${HEADERS})
find_library(METAL_LIBRARY Metal)
find_library(QUARTZ_LIBRARY QuartzCore)
target_link_libraries(halley-metal halley-core halley-utils "${METAL_LIBRARY}" "${QUARTZ_LIBRARY}")
