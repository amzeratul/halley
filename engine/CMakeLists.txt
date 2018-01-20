add_subdirectory(utils)
add_subdirectory(net)
add_subdirectory(entity)
add_subdirectory(audio)
add_subdirectory(core)
add_subdirectory(lua)
add_subdirectory(ui)

if (USE_DX11)
    add_subdirectory(dx11)
endif ()

if (USE_OPENGL)
    add_subdirectory(opengl)
endif ()

if (USE_SDL2)
    add_subdirectory(sdl)
endif ()

if (USE_ASIO)
    add_subdirectory(asio)
endif ()
