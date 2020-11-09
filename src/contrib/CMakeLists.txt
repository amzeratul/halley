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

        "yaml-cpp/binary.cpp"
        "yaml-cpp/convert.cpp"
        "yaml-cpp/depthguard.cpp"
        "yaml-cpp/directives.cpp"
        "yaml-cpp/emit.cpp"
        "yaml-cpp/emitfromevents.cpp"
        "yaml-cpp/emitter.cpp"
        "yaml-cpp/emitterstate.cpp"
        "yaml-cpp/emitterutils.cpp"
        "yaml-cpp/exceptions.cpp"
        "yaml-cpp/exp.cpp"
        "yaml-cpp/memory.cpp"
        "yaml-cpp/node.cpp"
        "yaml-cpp/nodebuilder.cpp"
        "yaml-cpp/nodeevents.cpp"
        "yaml-cpp/node_data.cpp"
        "yaml-cpp/null.cpp"
        "yaml-cpp/ostream_wrapper.cpp"
        "yaml-cpp/parse.cpp"
        "yaml-cpp/parser.cpp"
        "yaml-cpp/regex_yaml.cpp"
        "yaml-cpp/scanner.cpp"
        "yaml-cpp/scanscalar.cpp"
        "yaml-cpp/scantag.cpp"
        "yaml-cpp/scantoken.cpp"
        "yaml-cpp/simplekey.cpp"
        "yaml-cpp/singledocparser.cpp"
        "yaml-cpp/stream.cpp"
        "yaml-cpp/tag.cpp"

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

        "yaml-cpp/anchor.h"
        "yaml-cpp/binary.h"
        "yaml-cpp/collectionstack.h"
        "yaml-cpp/depthguard.h"
        "yaml-cpp/directives.h"
        "yaml-cpp/dll.h"
        "yaml-cpp/emitfromevents.h"
        "yaml-cpp/emitter.h"
        "yaml-cpp/emitterdef.h"
        "yaml-cpp/emittermanip.h"
        "yaml-cpp/emitterstate.h"
        "yaml-cpp/emitterstyle.h"
        "yaml-cpp/emitterutils.h"
        "yaml-cpp/eventhandler.h"
        "yaml-cpp/exceptions.h"
        "yaml-cpp/exp.h"
        "yaml-cpp/indentation.h"
        "yaml-cpp/mark.h"
        "yaml-cpp/nodebuilder.h"
        "yaml-cpp/nodeevents.h"
        "yaml-cpp/noexcept.h"
        "yaml-cpp/null.h"
        "yaml-cpp/ostream_wrapper.h"
        "yaml-cpp/parser.h"
        "yaml-cpp/ptr_vector.h"
        "yaml-cpp/regeximpl.h"
        "yaml-cpp/regex_yaml.h"
        "yaml-cpp/scanner.h"
        "yaml-cpp/scanscalar.h"
        "yaml-cpp/scantag.h"
        "yaml-cpp/setting.h"
        "yaml-cpp/singledocparser.h"
        "yaml-cpp/stlemitter.h"
        "yaml-cpp/stream.h"
        "yaml-cpp/streamcharsource.h"
        "yaml-cpp/stringsource.h"
        "yaml-cpp/tag.h"
        "yaml-cpp/token.h"
        "yaml-cpp/traits.h"
        "yaml-cpp/yaml.h"

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
