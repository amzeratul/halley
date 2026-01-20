#pragma once

#include "stats_view.h"

namespace Halley
{
	class ResourcesView : public StatsView
	{
	public:
		ResourcesView(Resources& resources, const HalleyAPI& api);

		void update(Time t) override;
		void paint(Painter& painter) override;

	private:
		Sprite boxBg;
		Sprite whitebox;
		TextRenderer text;

		AssetType assetType = AssetType::Texture;

		Colour4f getColour(ResourceDesiredLoadState loadState) const;
	};
}
