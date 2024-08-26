#pragma once
#include "halley/entity/service.h"
#include "halley/maths/colour.h"
#include "halley/time/halleytime.h"
#include "halley/game/frame_data.h"

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
		
		void startUpdate(Resources& resources, Time t);
		void startRender(bool waitForSpriteLoad = true);
		void endRender();

		BaseFrameData& getFrameData();
		bool hasFrameData() const;

		void draw(int mask, Painter& painter);
		void clear();

		void setBackgroundClearColour(std::optional<Colour4f> colour);
		const std::optional<Colour4f>& getClearColour() const { return clearColour; }

		void setDepthQueriesEnabled(bool enabled, std::optional<uint16_t> worldPartition);

		void setUpdateEnabled(bool enabled);
		bool isUpdateEnabled() const;

		void setRenderGraph(RenderGraph* renderGraph);

		std::shared_ptr<Texture> getRenderGraphTexture(const String& id) const;
		void setRenderTargetSize(const String& id, const Vector2i& size);

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
		RenderGraph* renderGraph = nullptr;
	};
}

using PainterService = Halley::PainterService;
