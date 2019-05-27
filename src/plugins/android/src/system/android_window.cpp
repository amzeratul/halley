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
    return Rect4i(Vector2i(), definition.getSize());
}

const WindowDefinition& AndroidWindow::getDefinition() const
{
    return definition;
}
