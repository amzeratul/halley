#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class HalleyAPI;
	class Project;
	class UIFactory;

	class SceneEditorWindow final : public UIWidget {
	public:
		SceneEditorWindow(UIFactory& factory, Project& project, const HalleyAPI& api);

	private:
		UIFactory& factory;
		Project& project;

		void makeUI();
	};
}
