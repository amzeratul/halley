project (halley-android)

include_directories(${Boost_INCLUDE_DIR} "../../engine/utils/include" "../../engine/core/include" "../../engine/audio/include" ${ANDROID_NDK}/sources/android/native_app_glue)

set(SOURCES
    "src/android_plugin.cpp"
    "src/android_system_api.cpp"
    "${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c"
    )

set(HEADERS
    "src/android_system_api.h"
    )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-android ${SOURCES} ${HEADERS})
target_link_libraries(halley-android halley-core halley-audio android log)
