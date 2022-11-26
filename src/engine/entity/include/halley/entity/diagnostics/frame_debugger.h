#pragma once

#include "stats_view.h"

namespace Halley
{
	class System;

	class FrameDebugger : public StatsView
	{
	public:
		FrameDebugger(Resources& resources, const HalleyAPI& api);
		~FrameDebugger() override;

		void update() override;
		void paint(Painter& painter) override;

	private:
		TextRenderer headerText;
	};
}
