project (halley-tools)

include_directories("../utils/include")

set(SOURCES
        "src/codegen/codegen.cpp"
        "src/codegen/codegen_tool.cpp"
        "src/codegen/component_schema.cpp"
        "src/codegen/message_schema.cpp"
        "src/codegen/system_schema.cpp"
        "src/distance_field/distance_field_generator.cpp"
        "src/distance_field/distance_field_tool.cpp"
        "src/make_font/font_face.cpp"
        "src/make_font/make_font_tool.cpp"
        "src/tool/cli_tool.cpp"
        )

add_executable (halley-tools ${SOURCES})