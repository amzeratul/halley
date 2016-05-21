project (halley-cmd)

find_Package(Freetype REQUIRED)
include_directories("../tools/include" "../utils/include")

set(SOURCES
        "src/main.cpp"
        )

add_executable (halley-cmd ${SOURCES})
target_link_libraries (halley-cmd halley-tools halley-utils ${FREETYPE_LIBRARIES} yaml-cpp stdc++fs pthread)