#pragma once

namespace Halley {
	class Project;

	class AssetsEditorWindow : public UIWidget {
    public:
        AssetsEditorWindow(UIFactory& factory, Project& project);

	private:
		UIFactory& factory;
		Project& project;
    };
}