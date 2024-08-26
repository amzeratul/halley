#pragma once
#include "halley/entity/service.h"
#include "halley/maths/colour.h"
#include "halley/time/halleytime.h"
#include "halley/game/frame_data.h"
#include "halley/graphics/sprite/sprite.h"

namespace Halley {
	class Texture;
	class RenderGraph;
	class Painter;
	class BaseFrameData;
	class Resources;

	class PainterService : public Service
	{
	public:
		PainterService();
		~PainterService() override;
		
		void startUpdate(Resources& resources, Time t);
		void startRender(bool waitForSpriteLoad = true);
		void endRender();

		BaseFrameData& getFrameData();
		bool hasFrameData() const;

		void draw(SpriteMaskBase mask, Painter& painter);
		void clear();

		void setBackgroundClearColour(std::optional<Colour4f> colour);
		const std::optional<Colour4f>& getClearColour() const { return clearColour; }

		void setDepthQueriesEnabled(bool enabled, std::optional<uint16_t> worldPartition);

		void setUpdateEnabled(bool enabled);
		bool isUpdateEnabled() const;

		void setRenderGraph(std::unique_ptr<RenderGraph> renderGraph);
		RenderGraph& getRenderGraph();
		bool hasRenderGraph() const;

		template <typename T>
		T& getPainter()
		{
			return *getFrameData().tryGetPainter<T>();
		}

	private:
		bool updateEnabled = true;
		bool depthQueriesEnabled = false;
		std::optional<uint16_t> worldPartition;
		std::optional<Colour4f> clearColour;
		std::unique_ptr<RenderGraph> renderGraph;
	};
}

using PainterService = Halley::PainterService;
