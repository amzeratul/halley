add_subdirectory(contrib)
add_subdirectory(engine)
add_subdirectory(plugins)

if (BUILD_HALLEY_TOOLS)
    find_package(ShaderConductor REQUIRED)
    add_subdirectory(tools)
endif()

add_subdirectory(tests)
