#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "android_gl_context.h"
using namespace Halley;

static EGLint contextAttribs[] = {
    EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
    EGL_NONE
};

AndroidGLContext::AndroidGLContext(AndroidGLContext* other)
    : config(other->config)
    , display(other->display)
    , shared(true)
    , ownsSurface(true)
{
    EGLint surfaceAttribs[] = {
        EGL_WIDTH, 1,
        EGL_HEIGHT, 1,
        EGL_NONE
    };
    surface = eglCreatePbufferSurface(display, config, surfaceAttribs);
    context = eglCreateContext(display, config, other->context, contextAttribs);
    if (!context) {
        throw Exception("Unable to create shared OpenGL context", HalleyExceptions::SystemPlugin);
    }
}

AndroidGLContext::AndroidGLContext(EGLConfig config, EGLDisplay display, EGLSurface surface)
    : config(config)
    , display(display)
    , surface(surface)
    , shared(false)
{
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (!context) {
        throw Exception("Unable to create OpenGL context", HalleyExceptions::SystemPlugin);
    }
}

AndroidGLContext::~AndroidGLContext()
{
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (ownsSurface) {
        eglDestroySurface(display, surface);
    }
    eglDestroyContext(display, context);
}

void AndroidGLContext::bind()
{
    eglMakeCurrent(display, surface, surface, context);
}

std::unique_ptr <GLContext> AndroidGLContext::createSharedContext()
{
    return std::make_unique<AndroidGLContext>(this);
}
