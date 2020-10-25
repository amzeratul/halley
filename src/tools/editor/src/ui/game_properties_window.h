#pragma once

#include <halley.hpp>

namespace Halley {
	class Project;

	class GamePropertiesWindow : public UIWidget
	{
	public:
		explicit GamePropertiesWindow(UIFactory& factory, Project& project);

		void onMakeUI() override;

	private:
		Project& project;
	};
}
