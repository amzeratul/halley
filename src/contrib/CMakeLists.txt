project (halley-utils)

include_directories("libogg/include" "libvorbis/include" "libvorbis/lib" "yaml-cpp/include")


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

        "yaml-cpp/src/binary.cpp"
        "yaml-cpp/src/convert.cpp"
        "yaml-cpp/src/depthguard.cpp"
        "yaml-cpp/src/directives.cpp"
        "yaml-cpp/src/emit.cpp"
        "yaml-cpp/src/emitfromevents.cpp"
        "yaml-cpp/src/emitter.cpp"
        "yaml-cpp/src/emitterstate.cpp"
        "yaml-cpp/src/emitterutils.cpp"
        "yaml-cpp/src/exceptions.cpp"
        "yaml-cpp/src/exp.cpp"
        "yaml-cpp/src/memory.cpp"
        "yaml-cpp/src/node.cpp"
        "yaml-cpp/src/nodebuilder.cpp"
        "yaml-cpp/src/nodeevents.cpp"
        "yaml-cpp/src/node_data.cpp"
        "yaml-cpp/src/null.cpp"
        "yaml-cpp/src/ostream_wrapper.cpp"
        "yaml-cpp/src/parse.cpp"
        "yaml-cpp/src/parser.cpp"
        "yaml-cpp/src/regex_yaml.cpp"
        "yaml-cpp/src/scanner.cpp"
        "yaml-cpp/src/scanscalar.cpp"
        "yaml-cpp/src/scantag.cpp"
        "yaml-cpp/src/scantoken.cpp"
        "yaml-cpp/src/simplekey.cpp"
        "yaml-cpp/src/singledocparser.cpp"
        "yaml-cpp/src/stream.cpp"
        "yaml-cpp/src/tag.cpp"

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

        "yaml-cpp/include/yaml-cpp/anchor.h"
        "yaml-cpp/include/yaml-cpp/binary.h"
        "yaml-cpp/include/yaml-cpp/depthguard.h"
        "yaml-cpp/include/yaml-cpp/dll.h"
        "yaml-cpp/include/yaml-cpp/emitfromevents.h"
        "yaml-cpp/include/yaml-cpp/emitter.h"
        "yaml-cpp/include/yaml-cpp/emitterdef.h"
        "yaml-cpp/include/yaml-cpp/emittermanip.h"
        "yaml-cpp/include/yaml-cpp/emitterstyle.h"
        "yaml-cpp/include/yaml-cpp/eventhandler.h"
        "yaml-cpp/include/yaml-cpp/exceptions.h"
        "yaml-cpp/include/yaml-cpp/mark.h"
        "yaml-cpp/include/yaml-cpp/noexcept.h"
        "yaml-cpp/include/yaml-cpp/null.h"
        "yaml-cpp/include/yaml-cpp/ostream_wrapper.h"
        "yaml-cpp/include/yaml-cpp/parser.h"
        "yaml-cpp/include/yaml-cpp/stlemitter.h"
        "yaml-cpp/include/yaml-cpp/traits.h"
        "yaml-cpp/include/yaml-cpp/yaml.h"

        "yaml-cpp/src/collectionstack.h"
        "yaml-cpp/src/contrib/yaml-cpp.natvis"
        "yaml-cpp/src/directives.h"
        "yaml-cpp/src/emitterstate.h"
        "yaml-cpp/src/emitterutils.h"
        "yaml-cpp/src/exp.h"
        "yaml-cpp/src/indentation.h"
        "yaml-cpp/src/nodebuilder.h"
        "yaml-cpp/src/nodeevents.h"
        "yaml-cpp/src/ptr_vector.h"
        "yaml-cpp/src/regex_yaml.h"
        "yaml-cpp/src/regeximpl.h"
        "yaml-cpp/src/scanner.h"
        "yaml-cpp/src/scanscalar.h"
        "yaml-cpp/src/scantag.h"
        "yaml-cpp/src/setting.h"
        "yaml-cpp/src/singledocparser.h"
        "yaml-cpp/src/stream.h"
        "yaml-cpp/src/streamcharsource.h"
        "yaml-cpp/src/stringsource.h"
        "yaml-cpp/src/tag.h"
        "yaml-cpp/src/token.h"

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
