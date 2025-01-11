#pragma once

#include "halley/entity/inspector.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
    class RemoteInspectorWindow : public UIWidget {
    public:
        RemoteInspectorWindow(UIFactory& factory, std::shared_ptr<DevConServerConnection> connection, const HalleyAPI& api);

        void onMakeUI() override;

        void onActiveChanged(bool active) override;
        void update(Time t, bool moved) override;

    private:
        UIFactory& factory;
        std::shared_ptr<DevConServerConnection> connection;
        const HalleyAPI& api;

        std::shared_ptr<InspectorServer> inspectorServer;
    };
}
