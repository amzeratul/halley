#include "polygon_gizmo.h"
#include "halley/core/graphics/painter.h"
using namespace Halley;

PolygonGizmo::PolygonGizmo(const String& componentName, const String& fieldName, const ConfigNode& options)
	: componentName(componentName)
	, fieldName(fieldName)
	, isOpenPolygon(options["isOpenPolygon"].asBool(false))
{
}

void PolygonGizmo::update(Time time, const SceneEditorInputState& inputState)
{
	for (size_t i = 0; i < handles.size(); ++i) {
		handles[i].update(inputState);
		vertices[i] = handles[i].getPosition();
	}
	writePointsIfNeeded();
}

void PolygonGizmo::draw(Painter& painter) const
{
	const auto zoom = getZoom();
	const auto col = Colour4f(0, 1, 0.5f);
	const auto highCol = Colour4f(1, 1, 1);
	
	painter.drawLine(vertices, 2.0f / zoom, col, !isOpenPolygon);
	
	for (const auto& h: handles) {
		painter.drawRect(getHandleRect(h.getPosition(), 12.0f), 1.0f / zoom, h.isOver() ? highCol : col);
	}
}

std::shared_ptr<UIWidget> PolygonGizmo::makeUI()
{
	return {};
}

void PolygonGizmo::onEntityChanged()
{
	vertices = readPoints();
	updateHandles();
}

VertexList PolygonGizmo::readPoints()
{
	VertexList result;
	const auto* data = getComponentData(componentName);
	if (data) {
		const auto& seq = (*data)[fieldName].asSequence();
		result.reserve(seq.size());
		for (const auto& p: seq) {
			result.push_back(p.asVector2f());
		}
	}
	lastStored = result;
	return result;
}

void PolygonGizmo::writePoints(const VertexList& ps)
{
	auto* data = getComponentData(componentName);
	if (data) {
		ConfigNode::SequenceType result;
		result.reserve(ps.size());

		for (const auto& p: ps) {
			result.push_back(ConfigNode(p));
		}
		(*data)[fieldName] = ConfigNode(std::move(result));
		
		markModified(componentName, fieldName);
	}
	lastStored = ps;
}

void PolygonGizmo::updateHandles()
{
	handles.resize(vertices.size());
	for (size_t i = 0; i < vertices.size(); ++i) {
		handles[i].setPosition(vertices[i]);
		handles[i].setBoundsCheck([this] (Vector2f pos, Vector2f mousePos) -> bool
		{
			return getHandleRect(pos, 14.0f).contains(mousePos);
		});
	}
}

Rect4f PolygonGizmo::getHandleRect(Vector2f pos, float size) const
{
	const auto offset = Vector2f(size, size) / (2.0f * getZoom());
	return Rect4f(pos - offset, pos + offset);
}

void PolygonGizmo::writePointsIfNeeded()
{
	if (lastStored != vertices) {
		writePoints(vertices);
	}
}
