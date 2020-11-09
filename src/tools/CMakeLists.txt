if (MSVC)
    if (VCPKG_TARGET_TRIPLET)
        add_definitions(-DUSE_VCPKG_YAML)
    else ()
        set(YAMLCPP_LIBRARY "")
    endif ()
endif ()

find_package(Boost COMPONENTS system filesystem REQUIRED)
find_Package(Freetype REQUIRED)

add_subdirectory(tools)
add_subdirectory(cmd)
add_subdirectory(editor)

if (BUILD_HALLEY_LAUNCHER)
    add_subdirectory(launcher)
endif()
