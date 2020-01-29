project (halley-entity)

include_directories(${Boost_INCLUDE_DIR} "include/halley/entity" "../utils/include" "../../../shared_gen/cpp")

set(SOURCES
        "src/component.cpp"
        "src/entity.cpp"
        "src/entity_factory.cpp"
        "src/family"
        "src/family_binding.cpp"
        "src/family_mask.cpp"
        "src/message.cpp"
        "src/system.cpp"
        "src/world.cpp"
        )

set(HEADERS
        "include/halley/halley_entity.h"

        "include/halley/entity/component.h"
        "include/halley/entity/entity.h"
        "include/halley/entity/family_binding.h"
        "include/halley/entity/family_extractor.h"
        "include/halley/entity/entity_factory.h"
        "include/halley/entity/entity_id.h"
        "include/halley/entity/family.h"
        "include/halley/entity/family_mask.h"
        "include/halley/entity/family_type.h"
        "include/halley/entity/message.h"
        "include/halley/entity/service.h"
        "include/halley/entity/system.h"
        "include/halley/entity/type_deleter.h"
        "include/halley/entity/world.h"

        "include/halley/entity/components/transform_2d_component.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-entity ${SOURCES} ${HEADERS})
target_link_libraries(halley-entity halley-utils)
