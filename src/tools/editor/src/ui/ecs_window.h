#pragma once

#include <halley.hpp>

namespace Halley {
	class ECSWindow : public UIWidget
	{
	public:
		explicit ECSWindow(UIFactory& factory);

		void onMakeUI() override;
	};
}
