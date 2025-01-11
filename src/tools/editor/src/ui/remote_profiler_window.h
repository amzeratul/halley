#pragma once

#include "halley/ui/ui_widget.h"

namespace Halley {
    class RemoteProfilerDisplay : public UIWidget {
    public:
        RemoteProfilerDisplay(UIFactory& factory, const HalleyAPI& api);

        void update(Time t, bool moved) override;
        void draw(UIPainter& painter) const override;
        void setProfileData(std::shared_ptr<ProfilerData> profileData);

        void setPage(int page);
        int getPage() const;
        int getNumPages() const;

    private:
        std::shared_ptr<PerformanceStatsView> statsView;
    };

    class RemoteProfilerWindow : public UIWidget {
    public:
        RemoteProfilerWindow(UIFactory& factory, std::shared_ptr<DevConServerConnection> connection, const HalleyAPI& api);

        void onMakeUI() override;

        void onActiveChanged(bool active) override;
        void update(Time t, bool moved) override;

    private:
        UIFactory& factory;
        std::shared_ptr<DevConServerConnection> connection;
        const HalleyAPI& api;

        bool isListening = false;
        std::optional<uint32_t> activeInterest;
        std::shared_ptr<ProfilerData> lastProfileData;
        std::shared_ptr<RemoteProfilerDisplay> display;

        void setListeningToProfile(bool listening);
        void onProfileData(std::shared_ptr<ProfilerData> data);
    };
}
