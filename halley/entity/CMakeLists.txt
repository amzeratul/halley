project (halley-entity)

include_directories("include/halley/entity" "../utils/include")

set(SOURCES
        "src/component.cpp"
        "src/entity.cpp"
        "src/family"
        "src/family_binding.cpp"
        "src/family_mask.cpp"
        "src/message"
        "src/prec"
        "src/system"
        "src/world"
        )

set(HEADERS
        "include/halley/entity/component.h"
        "include/halley/entity/entity.h"
        "include/halley/entity/entity_id.h"
        "include/halley/entity/family_binding.h"
        "include/halley/entity/family_extractor.h"
        "include/halley/entity/family.h"
        "include/halley/entity/family_mask.h"
        "include/halley/entity/family_type.h"
        "include/halley/entity/message.h"
        "include/halley/entity/prec.h"
        "include/halley/entity/system.h"
        "include/halley/entity/type_deleter.h"
        "include/halley/entity/world.h"
        "include/halley/halley_entity.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-entity ${SOURCES} ${HEADERS})
target_link_libraries(halley-entity halley-utils)
