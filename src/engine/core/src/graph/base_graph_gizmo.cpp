#include "halley/graph/base_graph_gizmo.h"

#include "halley/graph/base_graph.h"
#include "halley/graph/base_graph_renderer.h"
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

void BaseGraphGizmo::updateNodeAutoConnection(gsl::span<const GraphNodeId> nodeIds)
{
	pendingAutoConnections.clear();
	
	for (const auto id: nodeIds) {
		const auto& node = baseGraph->getNode(id);
		const size_t nPins = node.getPinConfiguration().size();
		for (size_t pinIdx = 0; pinIdx < nPins; ++pinIdx) {
			const bool empty = !node.getPin(pinIdx).hasConnection();
			const auto srcPinType = node.getPinType(static_cast<GraphPinId>(pinIdx));

			if (empty) {
				auto conn = findAutoConnectionForPin(id, static_cast<GraphPinId>(pinIdx), node.getPosition(), srcPinType, nodeIds);
				if (conn) {
					pendingAutoConnections.push_back(*conn);
				}
			}
		}
	}

	pruneConflictingAutoConnections();
}

void BaseGraphGizmo::pruneConflictingAutoConnections()
{
	std::sort(pendingAutoConnections.begin(), pendingAutoConnections.end());

	for (size_t i = 0; i < pendingAutoConnections.size(); ++i) {
		for (size_t j = i + 1; j < pendingAutoConnections.size(); ++j) {
			if (pendingAutoConnections[i].conflictsWith(pendingAutoConnections[j])) {
				pendingAutoConnections.erase(pendingAutoConnections.begin() + j);
			}
		}
	}
}

bool BaseGraphGizmo::finishAutoConnection()
{
	bool changed = false;
	for (const auto& conn: pendingAutoConnections) {
		changed = baseGraph->connectPins(conn.srcNode, conn.srcPin, conn.dstNode, conn.dstPin) || changed;
	}
	pendingAutoConnections.clear();
	return changed;
}

std::optional<BaseGraphGizmo::Connection> BaseGraphGizmo::findAutoConnectionForPin(GraphNodeId srcNodeId, GraphPinId srcPinIdx, Vector2f nodePos, GraphNodePinType srcPinType, gsl::span<const GraphNodeId> excludeIds) const
{
	float bestDistance = 100.0f;
	std::optional<Connection> bestPath;

	const size_t n  = baseGraph->getNumNodes();
	for (size_t i = 0; i < n; ++i) {
		auto& node = baseGraph->getNode(i);
		if (std_ex::contains(excludeIds, node.getId()))	{
			continue;
		}

		// Coarse distance test
		if ((node.getPosition() - nodePos).length() > 200.0f) {
			continue;
		}

		const size_t nPins = node.getPinConfiguration().size();
		for (size_t pinIdx = 0; pinIdx < nPins; ++pinIdx) {
			const bool empty = !node.getPin(pinIdx).hasConnection();
			const auto dstPinType = node.getPinType(static_cast<GraphPinId>(pinIdx));
			if (empty && srcPinType.canConnectTo(dstPinType)) {
				const auto srcPos = renderer->getPinPosition(basePos, baseGraph->getNode(srcNodeId), srcPinIdx, getZoom());
				const auto dstPos = renderer->getPinPosition(basePos, baseGraph->getNode(node.getId()), static_cast<GraphPinId>(pinIdx), getZoom());
				const float distance = (srcPos - dstPos).length();
				if (distance < bestDistance) {
					bestDistance = distance;
					bestPath = Connection{ srcNodeId, node.getId(), srcPinIdx, static_cast<GraphPinId>(pinIdx), srcPinType, dstPinType, srcPos, dstPos, distance };
				}
			}
		}
	}

	return bestPath;
}
