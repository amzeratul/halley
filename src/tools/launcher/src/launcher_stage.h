#pragma once

#include <halley.hpp>

namespace Halley {
	class LauncherStage : public Stage {
	public:
		void init() override;

		void onVariableUpdate(Time) override;
		void onRender(RenderContext&) const override;
	};
}