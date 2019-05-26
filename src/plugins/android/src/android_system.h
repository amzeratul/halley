#pragma once

#include <android_native_app_glue.h>
#include <cstdint>

namespace Halley {
    class AndroidSystem {
    public:
        AndroidSystem(struct android_app* androidAppState);
        ~AndroidSystem();
        static AndroidSystem& get();

        void run();
        struct android_app* getAndroidApp() const;

        void onAppCmd(int32_t cmd);

    private:
        struct android_app* androidAppState = nullptr;
        bool initWindow = false;
    };
}