project (halley-tools)

include_directories(${BOOST_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} "include" "../../engine/core/include" "../../engine/utils/include" "../../engine/audio/include" "../../engine/net/include" "../../contrib/libogg/include" "../../contrib/libvorbis/include")

set(SOURCES

    "src/assets/asset_collector.cpp"
    "src/assets/asset_importer.cpp"
    "src/assets/check_assets_task.cpp"
    "src/assets/delete_assets_task.cpp"
    "src/assets/import_assets_task.cpp"
    "src/assets/import_assets_database.cpp"
    "src/assets/import_tool.cpp"

    "src/assets/importers/animation_importer.cpp"
    "src/assets/importers/audio_event_importer.cpp"
    "src/assets/importers/audio_importer.cpp"
    "src/assets/importers/bitmap_font_importer.cpp"
    "src/assets/importers/codegen_importer.cpp"
    "src/assets/importers/config_importer.cpp"
    "src/assets/importers/copy_file_importer.cpp"
    "src/assets/importers/font_importer.cpp"
    "src/assets/importers/image_importer.cpp"
    "src/assets/importers/material_importer.cpp"
    "src/assets/importers/sprite_importer.cpp"
    "src/assets/importers/spritesheet_importer.cpp"
    "src/assets/importers/shader_importer.cpp"
    "src/assets/importers/texture_importer.cpp"

    "src/codegen/cpp/codegen_cpp.cpp"
    "src/codegen/cpp/cpp_class_gen.cpp"
    "src/codegen/codegen.cpp"
    "src/codegen/codegen_tool.cpp"
    "src/codegen/component_schema.cpp"
    "src/codegen/custom_type_schema.cpp"
    "src/codegen/message_schema.cpp"
    "src/codegen/system_schema.cpp"
    
    "src/distance_field/distance_field_generator.cpp"
    "src/distance_field/distance_field_tool.cpp"
    
    "src/file/filesystem.cpp"

    "src/make_font/font_face.cpp"
    "src/make_font/font_generator.cpp"
    "src/make_font/make_font_tool.cpp"

    "src/packer/asset_pack_manifest.cpp"
    "src/packer/asset_packer.cpp"
    "src/packer/asset_packer_task.cpp"
    "src/packer/asset_packer_tool.cpp"

    "src/tool/cli_tool.cpp"

	"src/sprites/aseprite_file.cpp"
    "src/sprites/aseprite_reader.cpp"

    "src/tasks/editor_task.cpp"
    "src/tasks/editor_task_set.cpp"

    "src/project/project.cpp"
    "src/project/project_loader.cpp"
    )

set(HEADERS
    "include/halley/plugin/halley_plugin.h"
    "include/halley/plugin/iasset_importer.h"

    "include/halley/tools/cli_tool.h"
    
    "include/halley/tools/codegen/codegen.h"
    "include/halley/tools/codegen/codegen_tool.h"
    "include/halley/tools/codegen/icode_generator.h"
    
    "include/halley/tools/distance_field/distance_field_generator.h"
    "include/halley/tools/distance_field/distance_field_tool.h"
    
    "include/halley/tools/file/filesystem.h"

    "include/halley/tools/make_font/font_face.h"
    "include/halley/tools/make_font/font_generator.h"
    "include/halley/tools/make_font/make_font_tool.h"

    "include/halley/tools/codegen/component_schema.h"
    "include/halley/tools/codegen/custom_type_schema.h"
    "include/halley/tools/codegen/fields_schema.h"
    "include/halley/tools/codegen/message_schema.h"
    "include/halley/tools/codegen/system_schema.h"
    
    "include/halley/tools/assets/asset_collector.h"
    "include/halley/tools/assets/asset_importer.h"
    "include/halley/tools/assets/check_assets_task.h"
    "include/halley/tools/assets/delete_assets_task.h"
    "include/halley/tools/assets/import_assets_task.h"
    "include/halley/tools/assets/import_assets_database.h"
    "include/halley/tools/assets/import_tool.h"

    "include/halley/tools/tasks/editor_task.h"
    "include/halley/tools/tasks/editor_task_set.h"

    "include/halley/tools/packer/asset_pack_manifest.h"
    "include/halley/tools/packer/asset_packer.h"
    "include/halley/tools/packer/asset_packer_task.h"
    "include/halley/tools/packer/asset_packer_tool.h"

    "include/halley/tools/project/project.h"
    "include/halley/tools/project/project_loader.h"

    "src/assets/importers/animation_importer.h"
    "src/assets/importers/audio_event_importer.h"
    "src/assets/importers/audio_importer.h"
    "src/assets/importers/bitmap_font_importer.h"
    "src/assets/importers/codegen_importer.h"
    "src/assets/importers/config_importer.h"
    "src/assets/importers/copy_file_importer.h"
    "src/assets/importers/font_importer.h"
    "src/assets/importers/image_importer.h"
    "src/assets/importers/material_importer.h"
    "src/assets/importers/sprite_importer.h"
    "src/assets/importers/spritesheet_importer.h"
    "src/assets/importers/shader_importer.h"
    "src/assets/importers/texture_importer.h"

    "src/codegen/cpp/codegen_cpp.h"
    "src/codegen/cpp/cpp_class_gen.h"

	"src/sprites/aseprite_file.h"
    "src/sprites/aseprite_reader.h"

    "src/yaml/halley-yamlcpp.h"
    )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-tools ${SOURCES} ${HEADERS} ${YAMLCPP_INCLUDE_DIR})

target_link_libraries (halley-tools
    halley-core
    halley-audio
    halley-net
    ${FREETYPE_LIBRARIES}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${YAMLCPP_LIBRARY}
    )