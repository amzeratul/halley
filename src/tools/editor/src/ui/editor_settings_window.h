#pragma once

#include <halley.hpp>

namespace Halley {
	class Preferences;

	class EditorSettingsWindow : public UIWidget
	{
	public:
		explicit EditorSettingsWindow(UIFactory& factory, Preferences& preferences);

		void onMakeUI() override;

	private:
		Preferences& preferences;
	};
}
