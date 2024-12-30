#pragma once

#include "halley/ui/ui_widget.h"

namespace Halley {
	class ProjectWindow;

	class RemotesWindow : public UIWidget {
    public:
        RemotesWindow(UIFactory& factory, ProjectWindow& projectWindow);

        void onMakeUI() override;

        void update(Time t, bool moved) override;

    private:
        UIFactory& factory;
        ProjectWindow& projectWindow;
        DevConServer& devConServer;

		std::shared_ptr<UIWidget> hasRemoteConnections;
		std::shared_ptr<UIWidget> noRemoteConnections;
        std::shared_ptr<UIList> tabs;
        std::shared_ptr<UIPagedPane> pages;

        Vector<size_t> curTabs;

        void updateTabs();
	};
}
