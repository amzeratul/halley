cmake_policy(SET CMP0054 NEW)

if (NOT CMAKE_VERSION VERSION_LESS 3.30)
    cmake_policy(SET CMP0167 OLD)
endif()

find_package(Boost REQUIRED)
find_Package(Freetype REQUIRED)

add_subdirectory(tools)
add_subdirectory(cmd)
add_subdirectory(editor)
