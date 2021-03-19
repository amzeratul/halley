project (halley-editor-extensions)

include_directories(${Boost_INCLUDE_DIR} "include" "include/halley/editor_extensions" "../core/include" "../utils/include" "../entity/include" "../ui/include" "../../../shared_gen/cpp")

set(SOURCES
        "src/component_editor_context.cpp"
        "src/component_field_parameters.cpp"
        "src/polygon_gizmo.cpp"
        "src/scene_editor.cpp"
        "src/scene_editor_gizmo.cpp"
        "src/vertex_gizmo.cpp"
        )

set(HEADERS
        "include/halley/editor_extensions/halley_editor_extensions.h"

        "include/halley/editor_extensions/component_editor_context.h"
        "include/halley/editor_extensions/component_field_parameters.h"
        "include/halley/editor_extensions/editor_custom_tools_interface.h"
        "include/halley/editor_extensions/polygon_gizmo.h"
        "include/halley/editor_extensions/scene_editor_input_state.h"
        "include/halley/editor_extensions/scene_editor.h"
        "include/halley/editor_extensions/scene_editor_gizmo.h"
        "include/halley/editor_extensions/vertex_gizmo.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-editor-extensions ${SOURCES} ${HEADERS})
target_link_libraries(halley-editor-extensions halley-core halley-utils halley-entity halley-ui)
