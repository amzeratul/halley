#pragma once

#include <halley.hpp>

namespace Halley {
	class EditorSettingsWindow : public UIWidget
	{
	public:
		explicit EditorSettingsWindow(UIFactory& factory);

		void onMakeUI() override;
	};
}
