#pragma once

#if defined(WITH_GDK)

#include "os_winbase.h"
#include <XUser.h>

namespace Halley {

    class OSGDK final : public OSWinBase
    {
    public:
        OSGDK();
        ~OSGDK() override;

        ComputerData getComputerData() override;
        String getUserDataDir() override;

        void openURL(const String& url) override;

    private:
        XUserHandle userHandle = nullptr;
        uint64_t userId = 0;
        ComputerData computerData = {};
    };

}

#endif
