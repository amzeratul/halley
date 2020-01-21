project (halley-utils)

include_directories("libogg/include" "libvorbis/include" "libvorbis/lib")


set(SOURCES
        "json/jsoncpp.cpp"

        "lodepng/lodepng.cpp"

        "speex/resample.c"

        "stb_image/stb_image.c"

        "tiny-aes/aes.c"

        "tinyxml/ticpp.cpp"
        "tinyxml/tinystr.cpp"
        "tinyxml/tinyxml.cpp"
        "tinyxml/tinyxmlerror.cpp"
        "tinyxml/tinyxmlparser.cpp"

        "xxhash/xxhash.c"

        "zlib/adler32.c"
        "zlib/compress.c"
        "zlib/crc32.c"
        "zlib/deflate.c"
        "zlib/infback.c"
        "zlib/inffast.c"
        "zlib/inflate.c"
        "zlib/inftrees.c"
        "zlib/trees.c"
        "zlib/uncompr.c"
        "zlib/zutil.c"
        )

set(HEADERS
        "json/json-forwards.h"
        "json/json.h"
        
        "lodepng/lodepng.h"
        
        "stb_image/stb_image.h"
        
        "tiny-aes/aes.h"
        "tiny-aes/aes.hpp"
        
        "tinyxml/ticpp.h"
        "tinyxml/ticpprc.h"
        "tinyxml/tinystr.h"
        "tinyxml/tinyxml.h"
        
        "speex/arch.h"
        "speex/fixed_generic.h"
        "speex/speex_resampler.h"
        "speex/stack_alloc.h"
        
        "xxhash/xxhash.h"

        "zlib/crc32.h"
        "zlib/deflate.h"
        "zlib/inffast.h"
        "zlib/inffixed.h"
        "zlib/inflate.h"
        "zlib/inftrees.h"
        "zlib/trees.h"
        "zlib/zconf.h"
        "zlib/zlib.h"
        "zlib/zutil.h"
        )

if(MSVC)
        add_definitions("-wd4996 -wd4244 -wd4100 -wd4267 -wd4189 -wd4305 -wd4127 -wd4703 -wd4706")
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

file (GLOB_RECURSE OGG_FILES "libogg/*.c")
file (GLOB_RECURSE VORBIS_FILES "libvorbis/*.c")
file (GLOB_RECURSE LUA_FILES "lua/src/*.*")
file (GLOB_RECURSE SELENE_FILES "selene/include/*.*")
set(SOURCES ${SOURCES} ${OGG_FILES} ${VORBIS_FILES} ${LUA_FILES} ${SELENE_FILES})

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-contrib ${SOURCES} ${HEADERS})
