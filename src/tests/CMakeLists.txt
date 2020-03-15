project (halley-tests)

include_directories(${Boost_INCLUDE_DIR} "include" "../core/include" "../utils/include" "../audio/include" "../net/include" "../entity/include")

set(SOURCES
        "src/path_test.cpp"
        )

set(HEADERS
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable (halley-tests ${SOURCES} ${HEADERS})
target_link_libraries(halley-tests halley-core halley-utils halley-audio halley-net halley-entity ${GTEST_BOTH_LIBRARIES})
add_test(halley-tests halley-tests)
