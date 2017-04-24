project (halley-lua)

include_directories(${Boost_INCLUDE_DIR} "include/halley/lua" "../utils/include" "../core/include" "../../contrib/lua/src" "../../contrib/selene/include")

set(SOURCES
        )

set(HEADERS
        "include/halley/lua/lua.h"
        )

file (GLOB_RECURSE LUA_FILES "../../contrib/lua/src/*.*")
file (GLOB_RECURSE SELENE_FILES "../../contrib/selene/include/*.*")

set(SOURCES ${SOURCES} ${LUA_FILES} ${SELENE_FILES})

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-lua ${SOURCES} ${HEADERS})
