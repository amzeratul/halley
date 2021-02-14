#pragma once

#include <halley.hpp>

namespace Halley {
	class ECSWindow : public UIWidget
	{
	public:
		explicit ECSWindow(UIFactory& factory, Project& project);

		void onMakeUI() override;
	
	private:
		UIFactory& factory;
		Project& project;

		void populateSystem(const String& name);
		void populateComponent(const String& name);
	};
}
