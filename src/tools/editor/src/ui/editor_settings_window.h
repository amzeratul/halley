#pragma once

#include <halley.hpp>
#include "src/preferences.h"

namespace Halley {
	class Project;
	class ProjectLoader;
	class Preferences;
	class EditorUIFactory;

	class EditorSettingsWindow : public UIWidget
	{
	public:
		explicit EditorSettingsWindow(EditorUIFactory& factory, Preferences& preferences, Project& project, ProjectLoader& projectLoader);

		void onMakeUI() override;

	private:
		EditorUIFactory& factory;
		Preferences& preferences;
		Project& project;
		ProjectLoader& projectLoader;

		Preferences workingCopy;

		void save();
		void reset();
		void setSaveEnabled(bool enabled);
	};
}
