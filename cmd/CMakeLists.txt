project (halley-cmd)

find_Package(Freetype REQUIRED)
find_package(YamlCpp REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIR} ${YAMLCPP_INCLUDE_DIR} "../tools/include" "../utils/include")

set(SOURCES "src/main.cpp")

add_executable (halley-cmd ${SOURCES})
target_link_libraries (halley-cmd halley-tools halley-utils ${FREETYPE_LIBRARIES} ${YAMLCPP_LIBRARY} stdc++fs pthread)