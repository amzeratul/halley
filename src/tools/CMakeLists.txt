cmake_policy(SET CMP0054 NEW)

find_package(Boost REQUIRED)
find_Package(Freetype REQUIRED)

add_subdirectory(tools)
add_subdirectory(cmd)
add_subdirectory(editor)
