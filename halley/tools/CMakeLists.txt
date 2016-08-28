project (halley-tools)

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
        "src/make_font/font_generator.cpp"
        "src/make_font/make_font_tool.cpp"

        "src/tool/cli_tool.cpp"

	"src/assets/check_assets_task.cpp"
	"src/assets/delete_assets_task.cpp"
	"src/assets/import_assets_task.cpp"
	"src/assets/import_assets_database.cpp"

        "src/tasks/editor_task.cpp"
	"src/tasks/editor_task_set.cpp"

       	"src/project/project.cpp"
        )

set(HEADERS
        "include/halley/tools/cli_tool.h"
        
        "include/halley/tools/codegen/codegen.h"
        "include/halley/tools/codegen/codegen_tool.h"
        "include/halley/tools/codegen/icode_generator.h"
        
        "include/halley/tools/distance_field/distance_field_generator.h"
        "include/halley/tools/distance_field/distance_field_tool.h"
        
        "include/halley/tools/make_font/font_face.h"
        "include/halley/tools/make_font/font_generator.h"
        "include/halley/tools/make_font/make_font_tool.h"

        "include/halley/tools/codegen/component_schema.h"
        "include/halley/tools/codegen/fields_schema.h"
        "include/halley/tools/codegen/message_schema.h"
        "include/halley/tools/codegen/system_schema.h"
        
        "src/codegen/cpp/codegen_cpp.h"
        "src/codegen/cpp/cpp_class_gen.h"

	"include/halley/tools/assets/check_assets_task.h"
	"include/halley/tools/assets/delete_assets_task.h"
	"include/halley/tools/assets/import_assets_task.h"
	"include/halley/tools/assets/import_assets_database.h"

	"include/halley/tools/tasks/editor_task.h"
	"include/halley/tools/tasks/editor_task_set.h"

        "include/halley/tools/project/project.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-tools ${SOURCES} ${HEADERS})

target_link_libraries (halley-tools
        ${FREETYPE_LIBRARIES}
        ${YAMLCPP_LIBRARY}
        )