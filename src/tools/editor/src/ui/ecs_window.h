#pragma once

#include <halley.hpp>

namespace Halley {
	class ECSWindow : public UIWidget
	{
	public:
		explicit ECSWindow(UIFactory& factory, Project& project);

		void onMakeUI() override;
	
	private:
		Project& project;
	};
}
