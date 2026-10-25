project (halley-httplib)

include_directories(${OPENSSL_INCLUDE_DIR} "../../engine/core/include" "../../contrib/zlib")

set(SOURCES
    "contrib/cpp-httplib/httplib.cc"

    "src/httplib_plugin.cpp"
    "src/httplib_server.cpp"
    "src/httplib_web_api.cpp"
    "src/httplib_http_request.cpp"
    )

set(HEADERS
    "contrib/cpp-httplib/httplib.h"

    "src/httplib_server.h"
    "src/httplib_web_api.h"
    "src/httplib_http_request.h"
)

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

if (USE_SSL)
    add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)
endif()

add_definitions(-DCPPHTTPLIB_ZLIB_SUPPORT)

add_library (halley-httplib ${SOURCES} ${HEADERS})
target_link_libraries(halley-httplib halley-engine ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})

# cpp-httplib calls some OpenSSL functions marked as deprecated, which fails the build with C4996
# on Windows with OpenSSL 4.1 installed.
target_compile_options(halley-httplib PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/wd4996>)
