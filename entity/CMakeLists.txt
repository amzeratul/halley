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


add_library (halley-entity ${SOURCES})
target_link_libraries(halley-entity halley-utils)
