project (halley-cmd)

find_Package(Freetype REQUIRED)
find_package(YamlCpp REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)
include_directories(${BOOST_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIR} ${YAMLCPP_INCLUDE_DIR} "../tools/include" "../utils/include")

set(SOURCES "src/main.cpp")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(EXTRA_LIBS)
else()
    set(EXTRA_LIBS pthread)
endif()

add_executable (halley-cmd ${SOURCES})
target_link_libraries (halley-cmd
        halley-tools
        halley-utils
        ${FREETYPE_LIBRARIES}
        ${YAMLCPP_LIBRARY}
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${EXTRA_LIBS}
        )
