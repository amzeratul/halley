#pragma once

namespace Halley {} // Get GitHub to realise this is C++ :3

#include "concurrency/concurrent.h"
#include "concurrency/jobs.h"
#include "concurrency/thread_pool.h"

#include "data_structures/circular_buffer.h"
#include "data_structures/dynamic_grid.h"
#include "data_structures/mapped_pool.h"
#include "data_structures/memory_pool.h"
#include "data_structures/rect_spatial_checker.h"

#include "file_formats/image.h"
#include "file_formats/ini_reader.h"
#include "file_formats/json_file.h"
#include "file_formats/serializer.h"
#include "file_formats/string_serializer.h"
#include "file_formats/text_file.h"
#include "file_formats/text_reader.h"
#include "file_formats/xml_file.h"
#include "file_formats/yaml_file.h"

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
#include "maths/utils.h"
#include "maths/vector2d.h"
#include "maths/vector3d.h"

#include "os/os.h"

#include "plugin/plugin.h"

#include "resources/resource.h"
#include "resources/resource_data.h"

#include "support/assert.h"
#include "support/console.h"
#include "support/debug.h"
#include "support/exception.h"
#include "support/redirect_stream.h"

#include "text/encode.h"
#include "text/halleystring.h"

#include "time/halleytime.h"
#include "time/stopwatch.h"

#include "web/http.h"
