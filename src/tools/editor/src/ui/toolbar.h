#pragma once

#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class EditorRootStage;
	class Project;
	class ProjectProperties;

	class Toolbar : public UIWidget
	{
	public:
		Toolbar(UIFactory& factory, EditorRootStage& editorStage, Project& project);

	private:
		UIFactory& factory;
		EditorRootStage& editorStage;
		Project& project;

		void makeUI();
	};
}
