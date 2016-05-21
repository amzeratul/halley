project (halley-utils)

find_package(Boost)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
    add_definitions("-DHAS_BOOST")
endif ()

set(SOURCES
        "src/concurrency/concurrent.cpp"
        "src/concurrency/jobs.cpp"
        "src/concurrency/thread_pool.cpp"
        "src/data_structures/bin_pack.cpp"
        "src/data_structures/highscore.cpp"
        "src/data_structures/memory_pool.cpp"
        "src/data_structures/rect_spatial_checker.cpp"
        "src/file_formats/image.cpp"
        "src/file_formats/ini_reader.cpp"
        "src/file_formats/json_file.cpp"
        "src/file_formats/json/jsoncpp.cpp"
        "src/file_formats/lodepng/lodepng.cpp"
        "src/file_formats/serializer.cpp"
        "src/file_formats/string_serializer.cpp"
        "src/file_formats/text_file.cpp"
        "src/file_formats/text_reader.cpp"
        "src/file_formats/tinyxml/ticpp.cpp"
        "src/file_formats/tinyxml/tinystr.cpp"
        "src/file_formats/tinyxml/tinyxml.cpp"
        "src/file_formats/tinyxml/tinyxmlerror.cpp"
        "src/file_formats/tinyxml/tinyxmlparser.cpp"
        "src/file_formats/xml_file.cpp"
        "src/file_formats/yaml_file.cpp"
        "src/maths/aabb.cpp"
        "src/maths/line.cpp"
        "src/maths/matrix4.cpp"
        "src/maths/polygon.cpp"
        "src/maths/random.cpp"
        "src/os/os_android.cpp"
        "src/os/os.cpp"
        "src/os/os_ios.cpp"
        "src/os/os_mac.cpp"
        "src/os/os_unix.cpp"
        "src/os/os_win32.cpp"
        "src/plugin/plugin.cpp"
        "src/resources/metadata.cpp"
        "src/resources/resource_data.cpp"
        "src/support/console.cpp"
        "src/support/debug.cpp"
        "src/support/exception.cpp"
        "src/support/redirect_stream.cpp"
        #"src/support/StackWalker/StackWalker.cpp"
        "src/text/encode.cpp"
        "src/text/halleystring.cpp"
        "src/time/stopwatch.cpp"
        "src/web/http.cpp"
        )

add_library (halley-utils ${SOURCES})