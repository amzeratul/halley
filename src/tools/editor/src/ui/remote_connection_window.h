#pragma once

#include "halley/ui/ui_widget.h"

namespace Halley {
	class ProjectWindow;

	class RemoteConnectionWindow : public UIWidget, public IUIDebugConsoleController {
    public:
        RemoteConnectionWindow(UIFactory& factory, ProjectWindow& projectWindow, std::shared_ptr<DevConServerConnection> connection);

        void onMakeUI() override;

        void update(Time t, bool moved) override;

	protected:
        Future<UIDebugConsoleResponse> runCommand(String command, Vector<String> args) override;
        Vector<StringUTF32> getAutoComplete(const StringUTF32& line) const override;

    private:
        UIFactory& factory;
        ProjectWindow& projectWindow;
        std::shared_ptr<DevConServerConnection> connection;

		std::shared_ptr<UIDebugConsole> console;
	};
}
