#pragma once

#include <halley.hpp>

namespace Halley {
	class GamePropertiesWindow : public UIWidget
	{
	public:
		explicit GamePropertiesWindow(UIFactory& factory);

		void onMakeUI() override;
	};
}
