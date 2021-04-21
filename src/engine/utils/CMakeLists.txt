project (halley-utils)

include_directories("include" "../core/include" "../../contrib/yaml-cpp/include" ${Boost_INCLUDE_DIR})

set(SOURCES
        "src/audio/resampler.cpp"
        
        "src/bytes/byte_serializer.cpp"
        "src/bytes/compression.cpp"
        "src/bytes/fuzzer.cpp"
        
        "src/concurrency/concurrent.cpp"
        "src/concurrency/executor.cpp"
        "src/concurrency/task.cpp"
        "src/concurrency/task_anchor.cpp"
        "src/concurrency/task_set.cpp"
        
        "src/data_structures/bin_pack.cpp"
        "src/data_structures/config_node.cpp"
        "src/data_structures/highscore.cpp"
        "src/data_structures/memory_pool.cpp"
        "src/data_structures/nullable_reference.cpp"
        "src/data_structures/rect_spatial_checker.cpp"
        
        "src/file/directory_monitor.cpp"
        "src/file/path.cpp"
        
        "src/file_formats/binary_file.cpp"
        "src/file_formats/config_file.cpp"
        "src/file_formats/ini_reader.cpp"
        "src/file_formats/json_file.cpp"
        "src/file_formats/image.cpp"
        "src/file_formats/text_file.cpp"
        "src/file_formats/text_reader.cpp"
        "src/file_formats/xml_file.cpp"
        "src/file_formats/yaml_convert.cpp"
        
        "src/maths/base_transform.cpp"
        "src/maths/bezier.cpp"
        "src/maths/circle.cpp"
        "src/maths/line.cpp"
        "src/maths/matrix4.cpp"
        "src/maths/mt199937ar.cpp"
        "src/maths/polygon.cpp"
        "src/maths/quaternion.cpp"
        "src/maths/random.cpp"
        "src/maths/ray.cpp"
        "src/maths/uuid.cpp"
        
        "src/memory/memory.cpp"
        
        "src/os/os_android.cpp"
        "src/os/os.cpp"
        "src/os/os_ios.cpp"
        "src/os/os_linux.cpp"
        "src/os/os_freebsd.cpp"
        "src/os/os_mac.cpp"
        "src/os/os_unix.cpp"
        "src/os/os_win32.cpp"

	"src/navigation/navigation_query.cpp"
	"src/navigation/navigation_path.cpp"
	"src/navigation/navigation_path_follower.cpp"
	"src/navigation/navmesh.cpp"
	"src/navigation/navmesh_generator.cpp"
	"src/navigation/navmesh_set.cpp"

        "src/resources/metadata.cpp"
        "src/resources/resource.cpp"
        "src/resources/resource_data.cpp"
        
        "src/support/console.cpp"
        "src/support/debug.cpp"
        "src/support/exception.cpp"
        "src/support/logger.cpp"
        "src/support/redirect_stream.cpp"
        "src/support/StackWalker/StackWalker.cpp"
        
        "src/text/encode.cpp"
        "src/text/fuzzy_text_matcher.cpp"
        "src/text/i18n.cpp"
        "src/text/halleystring.cpp"
        "src/text/string_serializer.cpp"
        
        "src/time/stopwatch.cpp"
        
        "src/utils/boost_system.cpp"
        "src/utils/encrypt.cpp"
        "src/utils/hash.cpp"
        "src/utils/variable.cpp"
        )

set(HEADERS
        "include/halley/audio/resampler.h"
        
        "include/halley/bytes/byte_serializer.h"
        "include/halley/bytes/config_node_serializer.h"
        "include/halley/bytes/config_node_serializer_base.h"
        "include/halley/bytes/compression.h"
        "include/halley/bytes/fuzzer.h"

        "include/halley/concurrency/concurrent.h"
        "include/halley/concurrency/executor.h"
        "include/halley/concurrency/future.h"
        "include/halley/concurrency/task.h"
        "include/halley/concurrency/task_anchor.h"
        "include/halley/concurrency/task_set.h"
        
        "include/halley/data_structures/bin_pack.h"
        "include/halley/data_structures/config_node.h"
        "include/halley/data_structures/config_node.natvis"
        "include/halley/data_structures/dynamic_grid.h"
        "include/halley/data_structures/flat_map.h"
        "include/halley/data_structures/hash_map.h"
        "include/halley/data_structures/highscore.h"
        "include/halley/data_structures/mapped_pool.h"
        "include/halley/data_structures/maybe.h"
        "include/halley/data_structures/maybe_ref.h"
        "include/halley/data_structures/memory_pool.h"
        "include/halley/data_structures/nullable_reference.h"
        "include/halley/data_structures/priority_queue.h"
        "include/halley/data_structures/rect_spatial_checker.h"
        "include/halley/data_structures/ring_buffer.h"
        "include/halley/data_structures/tree_map.h"
        "include/halley/data_structures/vector.h"
        
        "include/halley/file/directory_monitor.h"
        "include/halley/file/path.h"
        
        "include/halley/file_formats/binary_file.h"
        "include/halley/file_formats/config_file.h"
        "include/halley/file_formats/halley-yamlcpp.h"
        "include/halley/file_formats/image.h"
        "include/halley/file_formats/ini_reader.h"
        "include/halley/file_formats/json_file.h"
        "include/halley/file_formats/json_forward.h"
        "include/halley/file_formats/json/json.h"
        "include/halley/file_formats/text_file.h"
        "include/halley/file_formats/text_reader.h"
        "include/halley/file_formats/xml_file.h"
        "include/halley/file_formats/xml_forward.h"
        "include/halley/file_formats/yaml_convert.h"
        
        "include/halley/halley_json.h"
        "include/halley/halley_utils.h"
        "include/halley/maths/angle.h"
        "include/halley/maths/base_transform.h"
        "include/halley/maths/bezier.h"
        "include/halley/maths/box.h"
        "include/halley/maths/circle.h"
        "include/halley/maths/colour.h"
        "include/halley/maths/colour.natvis"
        "include/halley/maths/line.h"
        "include/halley/maths/matrix4.h"
        "include/halley/maths/polygon.h"
        "include/halley/maths/quaternion.h"
        "include/halley/maths/random.h"
        "include/halley/maths/ray.h"
        "include/halley/maths/uuid.h"
        "include/halley/maths/uuid.natvis"
        "src/maths/mt199937ar.h"
        "include/halley/maths/range.h"
        "include/halley/maths/rect.h"
        "include/halley/maths/simd.h"
        "include/halley/maths/tween.h"
        "include/halley/maths/vector2.h"
        "include/halley/maths/vector2.natvis"
        "include/halley/maths/vector3.h"
        "include/halley/maths/vector4.h"
        
        "include/halley/os/os.h"

	"include/halley/navigation/navigation_query.h"
	"include/halley/navigation/navigation_path.h"
	"include/halley/navigation/navigation_path_follower.h"
	"include/halley/navigation/navmesh.h"
	"include/halley/navigation/navmesh_generator.h"
	"include/halley/navigation/navmesh_set.h"
        
        "include/halley/plugin/plugin.h"
        
        "include/halley/resources/metadata.h"
        "include/halley/resources/resource_data.h"
        "include/halley/resources/resource.h"

        "include/halley/support/assert.h"
        "include/halley/support/console.h"
        "include/halley/support/debug.h"
        "include/halley/support/exception.h"
        "include/halley/support/logger.h"
        "include/halley/support/redirect_stream.h"

        "include/halley/text/encode.h"
        "include/halley/text/fuzzy_text_matcher"
        "include/halley/text/halleystring.h"
        "include/halley/text/halleystring.natvis"
        "include/halley/text/i18n.h"
        "include/halley/text/string_converter.h"
        "include/halley/text/string_serializer.h"

        "include/halley/time/halleytime.h"
        "include/halley/time/stopwatch.h"

        "include/halley/utils/algorithm.h"
        "include/halley/utils/attributes.h"
        "include/halley/utils/encrypt.h"
        "include/halley/utils/hash.h"
        "include/halley/utils/type_traits.h"
        "include/halley/utils/utils.h"
        "include/halley/utils/variable.h"

        "src/os/os_android.h"
        "src/os/os_ios.h"
        "src/os/os_linux.h"
        "src/os/os_mac.h"
        "src/os/os_unix.h"
        "src/os/os_win32.h"

        "src/support/StackWalker/StackWalker.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set_source_files_properties("src/maths/matrix4.cpp" PROPERTIES COMPILE_FLAGS "-msse3")
endif ()

add_library (halley-utils ${SOURCES} ${HEADERS})
target_precompile_headers(halley-utils PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/prec.h>")
target_link_libraries (halley-utils halley-contrib)
