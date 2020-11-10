project (halley-cmd)

include_directories(${BOOST_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIR} "../tools/include" "../../engine/utils/include" "../../engine/core/include" "../../contrib/yaml-cpp/include")
link_directories(${CMAKE_HOME_DIRECTORY}/lib)

set(SOURCES "src/main.cpp")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(EXTRA_LIBS bz2 z)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(EXTRA_LIBS pthread)
endif()

assign_source_group(${SOURCES})

add_executable (halley-cmd ${SOURCES})

target_link_libraries (halley-cmd
        halley-tools
        halley-utils
        halley-audio
        ${FREETYPE_LIBRARIES}
        ${YAMLCPP_LIBRARY}
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${EXTRA_LIBS}
        ${ShaderConductor_LIBRARY}
        )

get_property(deps GLOBAL PROPERTY halley_external_plugin_dependencies)
if (NOT ("${deps}" STREQUAL ""))
    separate_arguments(deps)
    add_dependencies(halley-cmd ${deps})
endif()
