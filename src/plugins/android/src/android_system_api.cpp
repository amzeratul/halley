#include "android_system_api.h"
#include "android_save_data.h"
#include "android_asset_reader.h"
#include "android_window.h"
#include "android_gl_context.h"
#include "android_system.h"
#include <android_native_app_glue.h>
#include <android/log.h>
#include <jni.h>
#include <string>
#include <EGL/egl.h>
#include <EGL/eglext.h>

using namespace Halley;

void AndroidSystemAPI::init()
{
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, 0, 0);

    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_CONFORMANT, EGL_OPENGL_ES3_BIT_KHR,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
    };
    EGLint numConfigs;
    eglChooseConfig(display, attribs, nullptr,0, &numConfigs);
    auto validConfigs = std::vector<EGLConfig>(numConfigs);
    eglChooseConfig(display, attribs, validConfigs.data(), numConfigs, &numConfigs);
    config = validConfigs[0];
    for (int i = 0; i < numConfigs; ++i) {
        auto& cfg = validConfigs[i];
        EGLint r, g, b, d;
        if (eglGetConfigAttrib(display, cfg, EGL_RED_SIZE, &r)   &&
            eglGetConfigAttrib(display, cfg, EGL_GREEN_SIZE, &g) &&
            eglGetConfigAttrib(display, cfg, EGL_BLUE_SIZE, &b)  &&
            eglGetConfigAttrib(display, cfg, EGL_DEPTH_SIZE, &d) &&
            r == 8 && g == 8 && b == 8 && d == 0 ) {

            config = validConfigs[i];
            break;
        }
    }

    surface = eglCreateWindowSurface(display, config, AndroidSystem::get().getAndroidApp()->window, nullptr);
}

void AndroidSystemAPI::deInit()
{
    eglDestroySurface(display, surface);
    eglTerminate(display);
}

Path AndroidSystemAPI::getAssetsPath(const Path& gamePath) const
{
    return ".";
}

Path AndroidSystemAPI::getUnpackedAssetsPath(const Path& gamePath) const
{
    return ".";
}

std::unique_ptr<ResourceDataReader> AndroidSystemAPI::getDataReader(String path, int64_t start, int64_t end)
{
    return std::make_unique<AndroidAssetReader>(AndroidSystem::get().getAndroidApp()->activity->assetManager, path);
}

std::unique_ptr<GLContext> AndroidSystemAPI::createGLContext()
{
    return std::make_unique<AndroidGLContext>(config, display, surface);
}

std::shared_ptr<Window> AndroidSystemAPI::createWindow(const WindowDefinition& window)
{
    return std::make_shared<AndroidWindow>(window, display, surface);
}

void AndroidSystemAPI::destroyWindow(std::shared_ptr<Window> window)
{
    // TODO
}

Vector2i AndroidSystemAPI::getScreenSize(int n) const
{
    // TODO
    return Vector2i(1920, 1080);
}

Rect4i AndroidSystemAPI::getDisplayRect(int screen) const
{
    return Rect4i(Vector2i(), getScreenSize(screen));
}

void AndroidSystemAPI::showCursor(bool show)
{
}

std::shared_ptr<ISaveData> AndroidSystemAPI::getStorageContainer(SaveDataType type, const String& containerName)
{
    return std::make_shared<AndroidSaveData>(type, containerName);
}

bool AndroidSystemAPI::generateEvents(VideoAPI* video, InputAPI* input)
{
    // TODO
    return true;
}
