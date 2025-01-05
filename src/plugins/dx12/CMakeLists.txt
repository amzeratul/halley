project (halley-dx12)

include_directories(${Boost_INCLUDE_DIR} "../../engine/utils/include" "../../engine/core/include")

set(SOURCES
        "src/dx12_descriptor_pool.cpp"
        "src/dx12_painter.cpp"
        "src/dx12_pipeline.cpp"
        "src/dx12_plugin.cpp"
        "src/dx12_render_target.cpp"
        "src/dx12_resource.cpp"
        "src/dx12_shader.cpp"
        "src/dx12_video.cpp"
)

set(HEADERS
        "src/dx12_descriptor_pool.h"
        "src/dx12_painter.h"
        "src/dx12_pipeline.h"
        "src/dx12_render_target.h"
        "src/dx12_resource.h"
        "src/dx12_shader.h"
        "src/dx12_video.h"
        "src/halley_dx12.h"
)

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-dx12 ${SOURCES} ${HEADERS})
target_link_libraries(halley-dx12 halley-engine)

if (_GAMING_XBOX_XBOXONE)
    target_include_directories(halley-dx12 PRIVATE $ENV{GXDKLatest}gameKit/Include/XboxOne)
elseif (_GAMING_XBOX_SCARLETT)
    target_include_directories(halley-dx12 PRIVATE $ENV{GXDKLatest}gameKit/Include/Scarlett)
endif ()
