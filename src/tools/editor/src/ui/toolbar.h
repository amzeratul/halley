#pragma once

#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class ProjectWindow;
	class Project;
	class ProjectProperties;

	class Toolbar : public UIWidget
	{
	public:
		Toolbar(UIFactory& factory, ProjectWindow& projectWindow, Project& project);

	private:
		UIFactory& factory;
		ProjectWindow& projectWindow;
		Project& project;

		void makeUI();
	};
}
