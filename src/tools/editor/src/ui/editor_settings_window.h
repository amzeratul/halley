#pragma once

#include <halley.hpp>
#include "src/preferences.h"

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
		UIFactory& factory;
		Preferences& preferences;
		Project& project;
		ProjectLoader& projectLoader;

		Preferences workingCopy;

		void save();
		void reset();
		void setSaveEnabled(bool enabled);
	};
}
