#pragma once

#include "stats_view.h"
#include "halley/core/graphics/render_snapshot.h"

namespace Halley
{
	class System;

	class FrameDebugger : public StatsView
	{
	public:
		FrameDebugger(Resources& resources, const HalleyAPI& api);
		~FrameDebugger() override;

		void update() override;
		void draw(RenderContext& context) override;
		void paint(Painter& painter) override;

		bool isRendering() const;

	private:
		TextRenderer headerText;

		std::unique_ptr<RenderSnapshot> renderSnapshot;
		bool waiting = false;
		
		size_t framesToDraw = 0;
	};
}
