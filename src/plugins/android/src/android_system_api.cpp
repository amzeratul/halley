#include "android_system_api.h"
#include <android/log.h>
#include <android_native_app_glue.h>
#include <jni.h>

using namespace Halley;

void AndroidSystemAPI::init()
{
    // TODO
}

void AndroidSystemAPI::deInit()
{
    // TODO
}

Path AndroidSystemAPI::getAssetsPath(const Path& gamePath) const
{
    // TODO
    return "";
}

Path AndroidSystemAPI::getUnpackedAssetsPath(const Path& gamePath) const
{
    // TODO
    return "";
}

std::unique_ptr<ResourceDataReader> AndroidSystemAPI::getDataReader(String path, int64_t start, int64_t end)
{
    // TODO
    return {};
}

std::unique_ptr<GLContext> AndroidSystemAPI::createGLContext()
{
    // TODO
    return {};
}

std::shared_ptr<Window> AndroidSystemAPI::createWindow(const WindowDefinition& window)
{
    // TODO
    return {};
}

void AndroidSystemAPI::destroyWindow(std::shared_ptr<Window> window)
{
    // TODO
}

Vector2i AndroidSystemAPI::getScreenSize(int n) const
{
    // TODO
    return Vector2i();
}

Rect4i AndroidSystemAPI::getDisplayRect(int screen) const
{
    // TODO
    return Rect4i();
}

void AndroidSystemAPI::showCursor(bool show)
{
    // TODO
}

std::shared_ptr<ISaveData> AndroidSystemAPI::getStorageContainer(SaveDataType type, const String& containerName)
{
    // TODO
    return {};
}

bool AndroidSystemAPI::generateEvents(VideoAPI* video, InputAPI* input)
{
    // TODO
    return true;
}

void android_main(struct android_app* state)
{
    // TODO
}
