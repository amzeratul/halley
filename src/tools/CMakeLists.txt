find_package(Boost COMPONENTS system filesystem REQUIRED)
find_Package(Freetype REQUIRED)

add_subdirectory(tools)
add_subdirectory(cmd)
add_subdirectory(editor)

if (BUILD_HALLEY_LAUNCHER)
    add_subdirectory(launcher)
endif()
