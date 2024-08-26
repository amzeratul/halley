#include "halley/entity/services/painter_service.h"
#include "halley/graphics/render_target/render_graph.h"
#include "halley/graphics/render_target/render_graph_node.h"

using namespace Halley;

PainterService::PainterService()
{
	clearColour = Colour4f::fromString("#B0B0B0");
	stencilClear = static_cast<uint8_t>(0);
	depthClear = 1.0f;
}

PainterService::~PainterService()
{
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

void PainterService::draw(SpriteMaskBase mask, Painter& painter)
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

void PainterService::setClearColour(std::optional<Colour4f> colour)
{
	clearColour = colour;
}

void PainterService::setStencilClear(std::optional<uint8_t> value)
{
	stencilClear = value;
}

void PainterService::setDepthClear(std::optional<float> value)
{
	depthClear = value;
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

void PainterService::setRenderGraph(std::unique_ptr<RenderGraph> renderGraph)
{
	this->renderGraph = std::move(renderGraph);
}

RenderGraph& PainterService::getRenderGraph()
{
	return *renderGraph;
}

bool PainterService::hasRenderGraph() const
{
	return !!renderGraph;
}
