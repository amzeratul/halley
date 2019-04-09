if (MSVC)
    set(YAMLCPP_LIBRARY "")
else ()
    find_package(YamlCpp)
endif ()

find_package(Boost COMPONENTS system filesystem REQUIRED)
find_Package(Freetype REQUIRED)

add_subdirectory(tools)
add_subdirectory(cmd)
add_subdirectory(editor)
add_subdirectory(editor-metal)
add_subdirectory(runner)
