project (halley-tests)

include_directories(
        ${Boost_INCLUDE_DIR}
        "include"
        "../../include"
        "../../src/engine/core/include"
        "../../src/engine/utils/include"
        "../../src/engine/audio/include"
        "../../src/engine/net/include"
        "../../src/engine/entity/include"
        "../../src/engine/lua/include"
        "../../src/engine/ui/include"
        "../../src/engine/editor_extensions/include"
)

set(SOURCES
        "src/config_node_test.cpp"
        "src/fuzzy_text_matcher_test.cpp"
        "src/path_test.cpp"
        "src/polygon_test.cpp"
        "src/serializer_test.cpp"
        "src/vector_test.cpp"
        )

set(HEADERS
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(halley-tests-exe ${SOURCES} ${HEADERS})
target_link_libraries(halley-tests-exe halley-engine ${GTEST_BOTH_LIBRARIES})
add_test(halley-tests COMMAND halley-tests)
