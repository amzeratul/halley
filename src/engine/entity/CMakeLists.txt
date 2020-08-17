project (halley-entity)

include_directories(${Boost_INCLUDE_DIR} "include" "include/halley/entity" "../core/include" "../utils/include" "../../../shared_gen/cpp")

set(SOURCES
        "src/component.cpp"
        "src/create_functions.cpp"
        "src/entity.cpp"
        "src/entity_factory.cpp"
        "src/entity_id.cpp"
        "src/entity_scene.cpp"
        "src/entity_stage.cpp"
        "src/family"
        "src/family_binding.cpp"
        "src/family_mask.cpp"
        "src/message.cpp"
        "src/prefab_scene_data.cpp"
        "src/system.cpp"
        "src/world.cpp"
        "src/world_scene_data.cpp"

        "src/components/transform_2d_component.cpp"

        "src/diagnostics/performance_stats.cpp"
        "src/diagnostics/stats_view.cpp"
        "src/diagnostics/world_stats.cpp"

        "src/scene_editor/scene_editor.cpp"
        )

set(HEADERS
        "include/halley/halley_entity.h"

        "include/halley/entity/component.h"
        "include/halley/entity/component_reflector.h"
        "include/halley/entity/create_functions.h"
        "include/halley/entity/entity.h"
        "include/halley/entity/family_binding.h"
        "include/halley/entity/family_extractor.h"
        "include/halley/entity/entity_factory.h"
        "include/halley/entity/entity_id.h"
        "include/halley/entity/entity_scene.h"
        "include/halley/entity/entity_stage.h"
        "include/halley/entity/family.h"
        "include/halley/entity/family_mask.h"
        "include/halley/entity/family_type.h"
        "include/halley/entity/message.h"
        "include/halley/entity/prefab_scene_data.h"
        "include/halley/entity/registry.h"
        "include/halley/entity/service.h"
        "include/halley/entity/system.h"
        "include/halley/entity/system_message.h"
        "include/halley/entity/type_deleter.h"
        "include/halley/entity/world.h"
        "include/halley/entity/world_scene_data.h"

        "include/halley/entity/components/transform_2d_component.h"

        "include/halley/entity/diagnostics/performance_stats.h"
        "include/halley/entity/diagnostics/stats_view.h"
        "include/halley/entity/diagnostics/world_stats.h"

        "include/halley/entity/scene_editor/scene_editor.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_definitions(-DDONT_INCLUDE_HALLEY_HPP)
add_library (halley-entity ${SOURCES} ${HEADERS})
target_link_libraries(halley-entity halley-utils halley-core)
