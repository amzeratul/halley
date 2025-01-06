#pragma once

#include "project_window.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
    class RemotesStatusBar : public UIWidget {
    public:
        RemotesStatusBar(UIFactory& factory, ProjectWindow& projectWindow);

        void onMakeUI() override;

        void update(Time t, bool moved) override;

		void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
        std::optional<MouseCursorMode> getMouseCursorMode() const override;

    private:
        UIFactory& factory;
        ProjectWindow& projectWindow;
        std::shared_ptr<UILabel> connectionsActive;

        Colour4f inactiveCol;
        Colour4f activeCol;
    };
}
