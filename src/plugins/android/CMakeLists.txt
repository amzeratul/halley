project (halley-android)

include_directories(${Boost_INCLUDE_DIR} "../../engine/utils/include" "../../engine/core/include" "../../engine/audio/include" ${ANDROID_NDK}/sources/android/native_app_glue)

set(SOURCES
    "src/android_system.cpp"
    "src/android_plugin.cpp"

    "src/input/android_input_api.cpp"
    "src/input/android_mouse.cpp"

    "src/system/android_asset_reader.cpp"
    "src/system/android_gl_context.cpp"
    "src/system/android_system_api.cpp"
    "src/system/android_save_data.cpp"
    "src/system/android_window.cpp"

    "${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c"
    )

set(HEADERS
    "src/android_system.h"

    "src/input/android_input_api.h"
    "src/input/android_mouse.h"
    
    "src/system/android_asset_reader.h"
    "src/system/android_gl_context.h"
    "src/system/android_system_api.h"
    "src/system/android_save_data.h"
    "src/system/android_window.h"
    )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-android ${SOURCES} ${HEADERS})
target_link_libraries(halley-android halley-core halley-audio android log EGL)
