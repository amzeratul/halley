#include "android_system.h"

#include <android/log.h>
#include <android_native_app_glue.h>
#include <jni.h>
#include <string>
#include <halley/runner/game_loader.h>
#include <halley/core/game/core.h>
#include <halley/runner/main_loop.h>
#include <halley/runner/entry_point.h>
#include "input/android_input_api.h"

using namespace Halley;

IHalleyEntryPoint* getHalleyEntry();

void android_main(struct android_app* state)
{
    auto system = AndroidSystem(state);
    system.run();
}

static AndroidSystem* sys = nullptr;

static int32_t onInputEvent(struct android_app* app, AInputEvent* event)
{
    AndroidSystem::get().onInputEvent(event);
    return 1;
}

static void onAppCmd(struct android_app* app, int32_t cmd)
{
    AndroidSystem::get().onAppCmd(cmd);
}

AndroidSystem::AndroidSystem(struct android_app* androidAppState)
    : androidAppState(androidAppState)
{
    sys = this;

    androidAppState->userData = this;
    androidAppState->onAppCmd = ::onAppCmd;
    androidAppState->onInputEvent = ::onInputEvent;
}

AndroidSystem::~AndroidSystem()
{
    sys = nullptr;
}

void AndroidSystem::run()
{
    std::vector<std::string> args = { "halleygame" };

    auto entry = getHalleyEntry();
    std::unique_ptr<IMainLoopable> core = entry->createCore(args);

    // Process events until we get window initialization
    int ident;
    int events;
    struct android_poll_source* source;
    while ((ident = ALooper_pollAll(-1, nullptr, &events, reinterpret_cast<void**>(&source))) >= 0) {
        if (source != nullptr) {
            source->process(androidAppState, source);
        }
        if (initWindow) {
            break;
        }
    }

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
}

struct android_app *AndroidSystem::getAndroidApp() const
{
    return androidAppState;
}

AndroidSystem& AndroidSystem::get()
{
    return *sys;
}

void AndroidSystem::onAppCmd(int32_t cmd)
{
    if (cmd == APP_CMD_INIT_WINDOW) {
        initWindow = true;
    }
}

void AndroidSystem::onInputEvent(AInputEvent *event)
{
    if (inputAPI) {
        inputAPI->onInputEvent(event);
    }
}

void AndroidSystem::setInputAPI(AndroidInputAPI *inputAPI)
{
    this->inputAPI = inputAPI;
}
