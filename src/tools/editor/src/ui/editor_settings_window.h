#pragma once

#include <halley.hpp>

namespace Halley {
	class Project;
	class ProjectLoader;
	class Preferences;

	class EditorSettingsWindow : public UIWidget
	{
	public:
		explicit EditorSettingsWindow(UIFactory& factory, Preferences& preferences, Project& project, ProjectLoader& projectLoader);

		void onMakeUI() override;

	private:
		Preferences& preferences;
		Project& project;
		ProjectLoader& projectLoader;
	};
}
