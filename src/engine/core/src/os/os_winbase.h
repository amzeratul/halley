#pragma once

#if defined(_WIN32) || defined(WITH_GDK)

#include <halley/os/os.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

namespace Halley {

    class OSWinBase : public OS
    {
    public:
        void createDirectories(const Halley::Path &path) override;
        Vector<Path> enumerateDirectory(const Halley::Path &path) override;
    };

}

#endif
