project (halley-vulkan)

include_directories( "../../engine/utils/include" "../../engine/core/include" ${Vulkan_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR} )

set(SOURCES
        "src/vulkan_video.cpp"
        "src/vulkan_plugin.cpp"
        )

set(HEADERS
        "src/vulkan_video.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-vulkan ${SOURCES} ${HEADERS})
target_link_libraries(halley-vulkan halley-engine)
