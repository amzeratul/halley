#include "android_system_api.h"
#include "android_save_data.h"
#include "android_asset_reader.h"
#include <android/log.h>
#include <android_native_app_glue.h>
#include <jni.h>
#include <halley/runner/game_loader.h>
#include <halley/core/game/core.h>
#include <halley/runner/main_loop.h>
#include <halley/runner/entry_point.h>
#include <string>

struct android_app* androidAppState = nullptr;

using namespace Halley;

void AndroidSystemAPI::init()
{

}

void AndroidSystemAPI::deInit()
{
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
    return std::make_unique<AndroidAssetReader>(androidAppState->activity->assetManager, path);
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
    return std::make_shared<AndroidSaveData>(type, containerName);
}

bool AndroidSystemAPI::generateEvents(VideoAPI* video, InputAPI* input)
{
    // TODO
    return true;
}

IHalleyEntryPoint* getHalleyEntry();

void android_main(struct android_app* state)
{
    androidAppState = state;
    __android_log_print(ANDROID_LOG_INFO, "halley-android", "Hello world!");

    std::vector<std::string> args = { "halleygame" };

    auto entry = getHalleyEntry();
    std::unique_ptr<IMainLoopable> core = entry->createCore(args);

    try {
        DummyGameLoader loader;
        loader.setCore(*core);
        core->getAPI().system->runGame([&]() {
            core->init();
            MainLoop loop(*core, loader);
            loop.run();
        });
    } catch (std::exception& e) {
        if (core) {
            core->onTerminatedInError(e.what());
        } else {
            std::cout << "Exception initialising core: " + String(e.what()) << std::endl;
        }
    } catch (...) {
        if (core) {
            core->onTerminatedInError("");
        } else {
            std::cout << "Unknown exception initialising core." << std::endl;
        }
    }

    __android_log_print(ANDROID_LOG_INFO, "halley-android", "Goodbye world!");
}
