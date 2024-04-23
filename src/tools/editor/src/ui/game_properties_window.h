#pragma once

#include <halley.hpp>

namespace Halley {
	class ProjectWindow;
	class Project;

	class GamePropertiesWindow : public UIWidget
	{
	public:
		explicit GamePropertiesWindow(UIFactory& factory, ProjectWindow& projectWindow);

		void onMakeUI() override;

	private:
		ProjectWindow& projectWindow;
	};
}
