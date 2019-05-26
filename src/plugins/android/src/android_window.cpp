#include "android_window.h"
using namespace Halley;

AndroidWindow::AndroidWindow(const WindowDefinition& definition, EGLDisplay display, EGLSurface surface)
    : definition(definition)
    , display(display)
    , surface(surface)
{

}

void AndroidWindow::update(const WindowDefinition& definition)
{

}

void AndroidWindow::show()
{

}

void AndroidWindow::hide()
{

}

void AndroidWindow::setVsync(bool vsync)
{

}

void AndroidWindow::swap()
{
    eglSwapBuffers(display, surface);
}

Rect4i AndroidWindow::getWindowRect() const
{
    // TODO
    return Rect4i(0, 0, 1920, 1080);
}

const WindowDefinition& AndroidWindow::getDefinition() const
{
    return definition;
}
