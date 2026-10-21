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

		int getNumPages() const override;
		int getPageNumber() const override;
		void setPageNumber(int page) override;

	private:
		struct Stats {
			String name;
			ResourceMemoryUsage usage;
			ResourceDesiredLoadState loadState;

			bool operator<(const Stats& other) const
			{
				if (loadState != other.loadState) {
					//return loadState < other.loadState;
				}
				return other.usage < usage;
			}
		};

		Sprite boxBg;
		Sprite whitebox;
		mutable TextRenderer text;

		AssetType assetType = AssetType::Texture;
		int pageNumber = 0;

		Colour4f getColour(ResourceDesiredLoadState loadState) const;
		Vector<Stats> getStats(AssetType type) const;
		
		void drawSummary(Painter& painter, Rect4f rect, const Vector<Stats>& stats) const;
		void drawStats(Painter& painter, Rect4f rect, const Vector<Stats>& stats) const;
	};
}
