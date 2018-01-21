add_subdirectory(engine)
add_subdirectory(plugins)

if (BUILD_HALLEY_TOOLS)
    add_subdirectory(tools)
endif()

if (BUILD_HALLEY_TESTS)
    add_subdirectory(tests)
endif()
