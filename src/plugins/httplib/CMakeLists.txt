project (halley-httplib)

include_directories(${Boost_INCLUDE_DIR} "../../engine/core/include")

set(SOURCES
    "contrib/cpp-httplib/httplib.cc"

    "src/httplib_plugin.cpp"
    "src/httplib_web_api.cpp"
    "src/httplib_http_request.cpp"
    )

set(HEADERS
    "contrib/cpp-httplib/httplib.h"

    "src/httplib_web_api.h"
    "src/httplib_http_request.h"
)

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-httplib ${SOURCES} ${HEADERS})
target_link_libraries(halley-httplib halley-engine)
