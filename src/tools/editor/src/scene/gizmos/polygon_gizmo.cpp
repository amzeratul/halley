#include "polygon_gizmo.h"
#include "halley/core/graphics/painter.h"
#include "halley/entity/components/transform_2d_component.h"
#include "halley/maths/line.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/widgets/ui_list.h"
using namespace Halley;

PolygonGizmo::PolygonGizmo(SnapRules snapRules, String componentName, String fieldName, const ConfigNode& options, UIFactory& factory)
	: SceneEditorGizmo(snapRules)
	, factory(factory)
	, componentName(std::move(componentName))
	, fieldName(std::move(fieldName))
	, isOpenPolygon(options["isOpenPolygon"].asBool(false))
	, colour(Colour4f::fromString(options["colour"].asString("#0080FF")))
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
			preview = snapVertex(static_cast<int>(handles.size()), inputState.mousePos);
		} else if (mode == PolygonGizmoMode::Insert) {
			std::tie(preview, previewIndex) = findInsertPoint(inputState.mousePos);
		}
	}

	// Insert/delete
	if (inputState.leftClickPressed) {
		if (mode == PolygonGizmoMode::Append && curFocus == -1) {
			handles.emplace_back(makeHandle(inputState.mousePos));
			setHandleIndices();
		} else if (mode == PolygonGizmoMode::Delete && curFocus != -1) {
			handles.erase(handles.begin() + curFocus);
			setHandleIndices();
		} else if (mode == PolygonGizmoMode::Insert && preview) {
			handles.insert(handles.begin() + previewIndex, makeHandle(preview.value()));
			setHandleIndices();
		}
	}

	// Update vertices
	vertices.resize(handles.size());
	for (size_t i = 0; i < handles.size(); ++i) {
		vertices[i] = worldToLocal(handles[i].getPosition());
	}
	
	writePointsIfNeeded();

	if (preview || curFocus != -1) {
		highlightCooldown = 3;
	} else if (highlightCooldown > 0) {
		--highlightCooldown;
	}
}

int PolygonGizmo::updateHandles(const SceneEditorInputState& inputState)
{
	enableLineSnap = inputState.shiftHeld;
	
	// Update existing handles
	for (auto& handle: handles) {
		handle.setCanDrag(mode != PolygonGizmoMode::Delete);
		handle.update(inputState, handles);
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
	const auto highCol = Colour4f(1, 1, 1);

	worldSpaceVertices.resize(vertices.size());
	for (size_t i = 0; i < vertices.size(); ++i) {
		worldSpaceVertices[i] = localToWorld(vertices[i]);
	}

	if (mode == PolygonGizmoMode::Append && preview.has_value()) {
		painter.drawLine(worldSpaceVertices, 2.0f / zoom, colour, false);

		const size_t nVertices = worldSpaceVertices.size();
		VertexList newBit;
		if (nVertices >= 1) {
			newBit.push_back(worldSpaceVertices.back());
		}
		newBit.push_back(preview.value());
		if (nVertices >= 2 && !isOpenPolygon) {
			newBit.push_back(worldSpaceVertices.front());
		}
		painter.drawLine(newBit, 1.0f / zoom, colour, false);
	} else {
		painter.drawLine(worldSpaceVertices, 2.0f / zoom, colour, !isOpenPolygon);
	}

	if (mode == PolygonGizmoMode::Insert && preview.has_value()) {
		painter.drawCircle(preview.value(), 3.0f / zoom, 2.0f, colour);
	}
	
	for (const auto& h: handles) {
		const float size = h.isSelected() ? 2.0f : 1.0f;
		painter.drawRect(getHandleRect(h.getPosition(), 12.0f), size / zoom, h.isOver() ? highCol : colour);
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

bool PolygonGizmo::isHighlighted() const
{
	return highlightCooldown > 0;
}

void PolygonGizmo::deselect()
{
	for (auto& handle: handles) {
		handle.setSelected(false);
	}
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
	auto* data = getComponentData(componentName);
	if (data) {
		auto& field = (*data)[fieldName];
		if (field.getType() != ConfigNodeType::Sequence) {
			field = ConfigNode::SequenceType();
		}
		
		const auto& seq = field.asSequence();
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
		handles[i].setPosition(localToWorld(vertices[i]), false);
	}
	setHandleIndices();
}

void PolygonGizmo::setHandleIndices()
{
	for (size_t i = 0; i < handles.size(); ++i) {
		handles[i].setId(static_cast<int>(i));
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

Vector2f PolygonGizmo::localToWorld(Vector2f localPos) const
{
	auto t = getTransform();
	if (t) {
		return t->transformPoint(localPos);
	} else {
		return localPos;
	}
}

Vector2f PolygonGizmo::worldToLocal(Vector2f worldPos) const
{
	auto t = getTransform();
	if (t) {
		return t->inverseTransformPoint(worldPos);
	} else {
		return worldPos;
	}
}

Vector2f PolygonGizmo::snapVertex(int id, Vector2f pos) const
{
	const auto rules = getSnapRules();

	if (!handles.empty() && enableLineSnap) {
		const auto* prev = tryGetHandle(modulo(id - 1, static_cast<int>(handles.size())));
		const auto* next = tryGetHandle(modulo(id + 1, static_cast<int>(handles.size())));
		pos = solveLineSnap(pos, prev ? prev->getPosition() : std::optional<Vector2f>(), next ? next->getPosition() : std::optional<Vector2f>());
	}
	
	if (rules.grid == GridSnapMode::Pixel) {
		pos = pos.round();
	}

	return pos;
}

const SceneEditorGizmoHandle* PolygonGizmo::tryGetHandle(int id) const
{
	for (const auto& handle: handles) {
		if (handle.getId() == id) {
			return &handle;
		}
	}
	return nullptr;
}

SceneEditorGizmoHandle PolygonGizmo::makeHandle(Vector2f pos) const
{
	SceneEditorGizmoHandle handle;
	handle.setBoundsCheck([this] (Vector2f pos, Vector2f mousePos) -> bool
	{
		return getHandleRect(pos, 14.0f).contains(mousePos);
	});

	handle.setSnap([this] (int id, Vector2f pos) { return snapVertex(id, pos); });
	
	handle.setPosition(pos, false);
	return handle;
}
