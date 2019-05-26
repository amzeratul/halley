#include "android_gl_context.h"
using namespace Halley;

AndroidGLContext::AndroidGLContext(AndroidGLContext* other)
    : shared(true)
    , config(other->config)
    , display(other->display)
    , surface(EGL_NO_SURFACE)
{
    context = eglCreateContext(other->display, other->config, other->context, nullptr);
}

AndroidGLContext::AndroidGLContext(EGLConfig config, EGLDisplay display, EGLSurface surface)
    : config(config)
    , display(display)
    , surface(surface)
    , shared(false)
{
    context = eglCreateContext(display, config, 0, nullptr);
}

AndroidGLContext::~AndroidGLContext()
{
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
