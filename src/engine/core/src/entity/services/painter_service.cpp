#include "halley/entity/services/painter_service.h"
#include "halley/graphics/render_target/render_graph.h"

using namespace Halley;

PainterService::PainterService()
{
	clearColour = Colour4f::fromString("#B0B0B0");
}

void PainterService::startUpdate(Resources& resources, Time t)
{
	for (auto& p: getFrameData().painters) {
		p->update(t, resources);
	}
}

void PainterService::startRender(bool waitForSpriteLoad)
{
	for (auto& p: getFrameData().painters) {
		p->startRender(waitForSpriteLoad, depthQueriesEnabled, worldPartition);
	}
}

void PainterService::endRender()
{
	for (auto& p: getFrameData().painters) {
		p->endRender();
	}
}

BaseFrameData& PainterService::getFrameData()
{
	return BaseFrameData::getCurrent();
}

bool PainterService::hasFrameData() const
{
	return BaseFrameData::hasCurrent();
}

void PainterService::draw(int mask, Painter& painter)
{
	for (auto& p: getFrameData().painters) {
		p->draw(mask, painter);
	}
}

void PainterService::clear()
{
	for (auto& p: getFrameData().painters) {
		p->clear();
	}
}

void PainterService::setBackgroundClearColour(std::optional<Colour4f> colour)
{
	clearColour = colour;
}

void PainterService::setDepthQueriesEnabled(bool enabled, std::optional<uint16_t> worldPartition)
{
	depthQueriesEnabled = enabled;
	this->worldPartition = worldPartition;
}

void PainterService::setUpdateEnabled(bool enabled)
{
	updateEnabled = enabled;
}

bool PainterService::isUpdateEnabled() const
{
	return updateEnabled;
}

void PainterService::setRenderGraph(RenderGraph* renderGraph)
{
	this->renderGraph = renderGraph;
}

std::shared_ptr<Texture> PainterService::getRenderGraphTexture(const String& id) const
{
	return renderGraph->getOutputTexture(id);
}

void PainterService::setRenderTargetSize(const String& id, const Vector2i& size)
{
	renderGraph->setRenderSize(id, size);
}
