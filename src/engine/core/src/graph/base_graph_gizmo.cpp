#include "halley/core/graph/base_graph_gizmo.h"

#include "halley/ui/ui_factory.h"
using namespace Halley;

bool BaseGraphGizmo::Connection::operator<(const Connection& other) const
{
	return distance < other.distance;
}

bool BaseGraphGizmo::Connection::conflictsWith(const Connection& other) const
{
	return (srcNode == other.srcNode && srcPin == other.srcPin) || (dstNode == other.dstNode && dstPin == other.dstPin);
}

void BaseGraphGizmo::setBasePosition(Vector2f pos)
{
	basePos = pos;
}

BaseGraphGizmo::BaseGraphGizmo(UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, Resources& resources, float baseZoom)
	: factory(factory)
	, entityEditorFactory(entityEditorFactory)
	, resources(&resources)
	, baseZoom(baseZoom)
{
	tooltipLabel
		.setFont(factory.getResources().get<Font>("Ubuntu Bold"))
		.setSize(14)
		.setColour(Colour(1, 1, 1))
		.setOutlineColour(Colour(0, 0, 0))
		.setOutline(1);
}

void BaseGraphGizmo::setUIRoot(UIRoot& root)
{
	uiRoot = &root;
}

void BaseGraphGizmo::setEventSink(UIWidget& sink)
{
	eventSink = &sink;
}

void BaseGraphGizmo::setZoom(float zoom)
{
	this->zoom = zoom;
}

float BaseGraphGizmo::getZoom() const
{
	return zoom;
}

void BaseGraphGizmo::setModifiedCallback(ModifiedCallback callback)
{
	modifiedCallback = std::move(callback);
}

void BaseGraphGizmo::onModified()
{
	if (modifiedCallback) {
		modifiedCallback();
	}
}
