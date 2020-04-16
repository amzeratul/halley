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
	
	const int curFocus = updateHandles(inputState);

	// Update preview
	preview.reset();
	if (curFocus == -1) {
		if (mode == PolygonGizmoMode::Append) {
			preview = inputState.mousePos;
		} else if (mode == PolygonGizmoMode::Insert) {
			std::tie(preview, previewIndex) = findInsertPoint(inputState.mousePos);
		}
	}

	// Insert/delete
	if (inputState.leftClickPressed) {
		if (mode == PolygonGizmoMode::Append && curFocus == -1) {
			handles.emplace_back(makeHandle(inputState.mousePos));
		} else if (mode == PolygonGizmoMode::Delete && curFocus != -1) {
			handles.erase(handles.begin() + curFocus);
		} else if (mode == PolygonGizmoMode::Insert && preview) {
			handles.insert(handles.begin() + previewIndex, makeHandle(preview.value()));
		}
	}

	// Update vertices
	vertices.resize(handles.size());
	for (size_t i = 0; i < handles.size(); ++i) {
		vertices[i] = handles[i].getPosition();
	}
	
	writePointsIfNeeded();
}

int PolygonGizmo::updateHandles(const SceneEditorInputState& inputState)
{
	// Update existing handles
	for (auto& handle: handles) {
		handle.setCanDrag(mode != PolygonGizmoMode::Delete);
		handle.update(inputState);
	}

	// Choose cur focus
	int curFocus = -1;
	const int nHandles = gsl::narrow_cast<int>(handles.size());
	for (int i = 0; i < nHandles; ++i) {
		if (handles[i].isHeld()) {
			curFocus = i;
		}
	}
	for (int i = 0; i < nHandles; ++i) {
		auto& handle = handles[i];
		if (handle.isOver()) {
			if (curFocus >= 0 && curFocus != i) {
				// Someone else already focused!
				handle.setNotOver();
			} else {
				curFocus = i;
			}
		}
	}

	return curFocus;
}

void PolygonGizmo::draw(Painter& painter) const
{
	const auto zoom = getZoom();
	const auto col = Colour4f(0, 1, 0.5f);
	const auto highCol = Colour4f(1, 1, 1);

	if (mode == PolygonGizmoMode::Append && preview.has_value()) {
		painter.drawLine(vertices, 2.0f / zoom, col, false);

		const size_t nVertices = vertices.size();
		VertexList newBit;
		if (nVertices >= 1) {
			newBit.push_back(vertices.back());
		}
		newBit.push_back(preview.value());
		if (nVertices >= 2 && !isOpenPolygon) {
			newBit.push_back(vertices.front());
		}
		painter.drawLine(newBit, 1.0f / zoom, col, false);
	} else {
		painter.drawLine(vertices, 2.0f / zoom, col, !isOpenPolygon);
	}

	if (mode == PolygonGizmoMode::Insert && preview.has_value()) {
		painter.drawCircle(preview.value(), 3.0f / zoom, 2.0f, col);
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
	
	loadHandlesFromVertices();
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

void PolygonGizmo::loadHandlesFromVertices()
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

std::pair<Vector2f, size_t> PolygonGizmo::findInsertPoint(Vector2f pos) const
{
	if (vertices.empty()) {
		return std::make_pair(pos, 0);
	}

	const size_t nVertices = vertices.size();
	const size_t n = isOpenPolygon ? nVertices - 1 : nVertices;

	size_t bestIndex = 0;
	Vector2f bestPoint;
	float bestDist = std::numeric_limits<float>::infinity();
	for (size_t i = 0; i < n; ++i) {
		const auto seg = LineSegment(vertices[i], vertices[(i + 1) % nVertices]);
		const auto p = seg.getClosestPoint(pos);
		const float dist = (p - pos).squaredLength();
		if (dist < bestDist) {
			bestDist = dist;
			bestIndex = i + 1;
			bestPoint = p;
		}
	}

	return std::make_pair(bestPoint, bestIndex);
}

SceneEditorGizmoHandle PolygonGizmo::makeHandle(Vector2f pos) const
{
	SceneEditorGizmoHandle handle;
	handle.setBoundsCheck([this] (Vector2f pos, Vector2f mousePos) -> bool
	{
		return getHandleRect(pos, 14.0f).contains(mousePos);
	});
	handle.setPosition(pos);
	return handle;
}
