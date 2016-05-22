project (halley-tools)

find_Package(Freetype REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)
include_directories(${BOOST_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} "include" "../utils/include")

set(SOURCES
        "src/codegen/cpp/codegen_cpp.cpp"
        "src/codegen/cpp/cpp_class_gen.cpp"
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

set(HEADERS
        "include/halley/tools/cli_tool.h"
        "include/halley/tools/codegen/codegen.h"
        "include/halley/tools/codegen/codegen_tool.h"
        "include/halley/tools/distance_field/distance_field_generator.h"
        "include/halley/tools/distance_field/distance_field_tool.h"
        "include/halley/tools/make_font/make_font_tool.h"
        "src/codegen/component_schema.h"
        "src/codegen/cpp/codegen_cpp.h"
        "src/codegen/cpp/cpp_class_gen.h"
        "src/codegen/fields_schema.h"
        "src/codegen/icode_generator.h"
        "src/codegen/message_schema.h"
        "src/codegen/system_schema.h"
        "src/make_font/font_face.h"
        "src/prec.h"
        )

add_library (halley-tools ${SOURCES} ${HEADERS})