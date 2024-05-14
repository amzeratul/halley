#pragma once

#include "stats_view.h"

namespace Halley
{
	class AudioView : public StatsView
	{
	public:
		AudioView(Resources& resources, const HalleyAPI& api);
		~AudioView() override;
		
		void paint(Painter& painter) override;

	private:
		Sprite boxBg;
		Sprite whitebox;
		TextRenderer headerText;
	};
}
