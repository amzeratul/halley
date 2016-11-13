#pragma once

namespace Halley {} // Get GitHub to realise this is C++ :3

#include "concurrency/concurrent.h"

#include "data_structures/bin_pack.h"
#include "data_structures/circular_buffer.h"
#include "data_structures/dynamic_grid.h"
#include "data_structures/hash_map.h"
#include "data_structures/mapped_pool.h"
#include "data_structures/maybe.h"
#include "data_structures/memory_pool.h"
#include "data_structures/rect_spatial_checker.h"
#include "data_structures/tree_map.h"
#include "data_structures/vector.h"

#include "file/byte_serializer.h"
#include "file/directory_monitor.h"
#include "file/string_serializer.h"

#include "file_formats/config_file.h"
#include "file_formats/image.h"
#include "file_formats/ini_reader.h"
#include "file_formats/json_file.h"
#include "file_formats/text_file.h"
#include "file_formats/text_reader.h"
#include "file_formats/xml_file.h"

#include "maths/aabb.h"
#include "maths/angle.h"
#include "maths/base_transform.h"
#include "maths/box.h"
#include "maths/colour.h"
#include "maths/matrix4.h"
#include "maths/polygon.h"
#include "maths/random.h"
#include "maths/range.h"
#include "maths/rect.h"
#include "maths/vector2.h"
#include "maths/vector3.h"
#include "maths/vector4.h"

#include "os/os.h"

#include "plugin/plugin.h"

#include "resources/metadata.h"
#include "resources/resource.h"
#include "resources/resource_data.h"

#include "support/assert.h"
#include "support/console.h"
#include "support/debug.h"
#include "support/exception.h"
#include "support/logger.h"
#include "support/redirect_stream.h"

#include "text/encode.h"
#include "text/halleystring.h"
#include "text/string_converter.h"

#include "time/halleytime.h"
#include "time/stopwatch.h"

#include "utils/algorithm.h"
#include "utils/type_traits.h"
#include "utils/utils.h"
