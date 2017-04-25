project (halley-lua)

include_directories(${Boost_INCLUDE_DIR} "include/halley/lua" "../utils/include" "../core/include" "../../contrib/lua/src")

set(SOURCES
        "src/lua_function_bind.cpp"
        "src/lua_reference.cpp"
        "src/lua_stack_ops.cpp"
        "src/lua_state.cpp"
        )

set(HEADERS
        "include/halley/lua/halley_lua.h"
        "include/halley/lua/lua_function_bind.h"
        "include/halley/lua/lua_reference.h"
        "include/halley/lua/lua_stack_ops.h"
        "include/halley/lua/lua_state.h"
        )

file (GLOB_RECURSE LUA_FILES "../../contrib/lua/src/*.*")
file (GLOB_RECURSE SELENE_FILES "../../contrib/selene/include/*.*")

set(SOURCES ${SOURCES} ${LUA_FILES} ${SELENE_FILES})

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-lua ${SOURCES} ${HEADERS})
