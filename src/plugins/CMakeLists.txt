if (USE_DX11)
    add_subdirectory(dx11)
endif ()

if (USE_OPENGL OR USE_OPENGL_ES2 OR USE_OPENGL_ES3)
    add_subdirectory(opengl)
endif ()

if (USE_METAL)
    add_subdirectory(metal)
endif ()

if (USE_SDL2)
    add_subdirectory(sdl)
endif ()

if (USE_ASIO)
    add_subdirectory(asio)
endif ()

if (USE_WINRT)
    add_subdirectory(winrt)
endif ()

if (USE_MEDIA_FOUNDATION)
    add_subdirectory(mf)
endif ()

if (USE_AVFOUNDATION)
    add_subdirectory(avf)
endif ()

if (USE_ANDROID)
    add_subdirectory(android)
endif ()
