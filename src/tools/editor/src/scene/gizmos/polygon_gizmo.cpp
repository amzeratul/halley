#include "polygon_gizmo.h"
#include "halley/core/graphics/painter.h"
using namespace Halley;

PolygonGizmo::PolygonGizmo(const String& componentName, const String& fieldName, const ConfigNode& options, UIFactory& factory)
	: factory(factory)
	, componentName(componentName)
	, fieldName(fieldName)
	, isOpenPolygon(options["isOpenPolygon"].asBool(false))
{
}

void PolygonGizmo::update(Time time, const SceneEditorInputState& inputState)
{
	if (inputState.rightClickPressed) {
		setMode(PolygonGizmoMode::Move);
	}
	
	// Update existing handles
	for (auto& handle: handles) {
		handle.setCanDrag(mode == PolygonGizmoMode::Move);
		handle.update(inputState);
	}

	// Insert/delete
	if (inputState.leftClickPressed) {
		if (mode == PolygonGizmoMode::Append) {
			handles.emplace_back(makeHandle(inputState.mousePos));
		}
	}

	// Update vertices
	vertices.resize(handles.size());
	for (size_t i = 0; i < handles.size(); ++i) {
		vertices[i] = handles[i].getPosition();
	}

	// Update preview vertex
	if (mode == PolygonGizmoMode::Append) {
		preview = inputState.mousePos;
	}
	
	writePointsIfNeeded();
}

void PolygonGizmo::draw(Painter& painter) const
{
	const auto zoom = getZoom();
	const auto col = Colour4f(0, 1, 0.5f);
	const auto highCol = Colour4f(1, 1, 1);

	if (mode == PolygonGizmoMode::Append) {
		painter.drawLine(vertices, 2.0f / zoom, col, false);

		const size_t nVertices = vertices.size();
		VertexList newBit;
		if (nVertices >= 1) {
			newBit.push_back(vertices.back());
		}
		newBit.push_back(preview);
		if (nVertices >= 2 && !isOpenPolygon) {
			newBit.push_back(vertices.front());
		}
		painter.drawLine(newBit, 1.0f / zoom, col, false);
	} else {
		painter.drawLine(vertices, 2.0f / zoom, col, !isOpenPolygon);
	}
	
	for (const auto& h: handles) {
		painter.drawRect(getHandleRect(h.getPosition(), 12.0f), 1.0f / zoom, h.isOver() ? highCol : col);
	}
}

std::shared_ptr<UIWidget> PolygonGizmo::makeUI()
{
	auto ui = factory.makeUI("ui/halley/polygon_gizmo_toolbar");
	uiList = ui->getWidgetAs<UIList>("mode");
	uiList->setSelectedOptionId(toString(mode));
	ui->setHandle(UIEventType::ListSelectionChanged, "mode", [=] (const UIEvent& event)
	{
		setMode(fromString<PolygonGizmoMode>(event.getStringData()));
	});
	return ui;
}

void PolygonGizmo::onEntityChanged()
{
	vertices = readPoints();

	mode = vertices.empty() ? PolygonGizmoMode::Append : PolygonGizmoMode::Move;
	
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
	handles.resize(vertices.size(), makeHandle({}));
	for (size_t i = 0; i < vertices.size(); ++i) {
		handles[i].setPosition(vertices[i]);
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

void PolygonGizmo::setMode(PolygonGizmoMode m)
{
	mode = m;
	if (uiList) {
		uiList->setSelectedOptionId(toString(mode));
	}
}

SceneEditorGizmoHandle PolygonGizmo::makeHandle(Vector2f pos)
{
	SceneEditorGizmoHandle handle;
	handle.setBoundsCheck([this] (Vector2f pos, Vector2f mousePos) -> bool
	{
		return getHandleRect(pos, 14.0f).contains(mousePos);
	});
	handle.setPosition(pos);
	return handle;
}
