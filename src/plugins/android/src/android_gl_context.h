#pragma once

#include <halley/core/api/system_api.h>
#include <EGL/egl.h>

namespace Halley {
    class AndroidGLContext : public GLContext {
    public:
        AndroidGLContext(AndroidGLContext* shared);
        AndroidGLContext(EGLConfig config, EGLDisplay display, EGLSurface surface);
        ~AndroidGLContext();

        void bind() override;
        std::unique_ptr<GLContext> createSharedContext() override;

    private:
        EGLConfig config;
        EGLDisplay display;
        EGLSurface surface;
        EGLContext context;
        bool shared = false;
    };
}
