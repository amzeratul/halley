#include "translate_gizmo.h"

#include <components/sprite_component.h>

#include "halley/entity/components/transform_2d_component.h"
#include "halley/core/game/scene_editor_interface.h"
#include "halley/core/graphics/painter.h"
using namespace Halley;

TranslateGizmo::TranslateGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow)
	: SceneEditorGizmo(snapRules)
	, factory(factory)
	, sceneEditorWindow(sceneEditorWindow)
{
	handle.setBoundsCheck([=] (Vector2f myPos, Vector2f mousePos) -> bool
	{
		return getMainHandle().contains(mousePos);
	});
	handle.setGridSnap(snapRules.grid);
}

void TranslateGizmo::update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState)
{
	const auto curMode = fromString<TranslateGizmoMode>(sceneEditorWindow.getSetting(EditorSettingType::Editor, "tools.translate.mode").asString("pivot"));
	if (curMode != mode) {
		setMode(curMode);
	}
	
	handle.update(inputState);

	const auto transform = getComponent<Transform2DComponent>();
	if (transform) {
		if (handle.isHeld()) {
			// Write to object
			transform->setGlobalPosition(handle.getPosition() - handleOffset);
			updateEntityData(transform->getLocalPosition());
		} else {
			// Read from object
			handleOffset = getObjectOffset();			
			handle.setPosition(transform->getGlobalPosition() + handleOffset, false);
		}
		visible = true;
	} else {
		visible = false;
	}
}

void TranslateGizmo::draw(Painter& painter) const
{
	if (visible) {
		const float zoom = getZoom();
		const auto overCol = Colour4f(0.6f, 0.6f, 1);
		const auto outCol = Colour4f(0.4f, 0.4f, 1.0f);
		const auto col = handle.isOver() ? overCol : outCol;
		const auto circle = getMainHandle();

		const auto centre = circle.getCentre();
		const auto radius = circle.getRadius();
		const float lineWidth = 2.0f / zoom;
		const float fineLineWidth = 1.0f / zoom;
		
		painter.drawCircle(centre, radius, lineWidth + 2 / zoom, Colour4f(0, 0, 0, 0.5f));
		painter.drawCircle(centre, radius, lineWidth, col);
		painter.drawLine({{ centre - Vector2f(radius * 0.6f, 0), centre + Vector2f(radius * 0.6f, 0) }}, fineLineWidth, col);
		painter.drawLine({{ centre - Vector2f(0, radius * 0.6f), centre + Vector2f(0, radius * 0.6f) }}, fineLineWidth, col);
	}
}

bool TranslateGizmo::isHighlighted() const
{
	return handle.isOver();
}

std::shared_ptr<UIWidget> TranslateGizmo::makeUI()
{
	auto ui = factory.makeUI("ui/halley/translate_gizmo_toolbar");
	ui->setInteractWithMouse(true);

	uiMode = ui->getWidgetAs<UIList>("mode");

	const auto initialMode = sceneEditorWindow.getSetting(EditorSettingType::Editor, "tools.translate.mode").asString("pivot");
	setMode(fromString<TranslateGizmoMode>(initialMode));
	ui->bindData("mode", initialMode, [=] (const String& value)
	{
		setMode(fromString<TranslateGizmoMode>(value));
	});
	
	return ui;
}

std::vector<String> TranslateGizmo::getHighlightedComponents() const
{
	return { "Transform2D" };
}

Circle TranslateGizmo::getMainHandle() const
{
	const auto pos = handle.getPosition();
	return Circle(pos, 10.0f / getZoom());
}

void TranslateGizmo::updateEntityData(Vector2f pos)
{
	auto* data = getComponentData("Transform2D");
	if (data) {
		(*data)["position"] = pos;
	}
	markModified("Transform2D", "position");
}

Vector2f TranslateGizmo::getObjectOffset() const
{
	if (mode == TranslateGizmoMode::Centre) {
		auto sprite = getComponent<SpriteComponent>();
		if (sprite) {
			auto offset = sprite->sprite.getAABB().getCenter() - sprite->sprite.getPosition();
			if (getSnapRules().grid == GridSnapMode::Pixel) {
				offset = offset.round();
			}
			return offset;
		}		
	}
	return Vector2f();
}

void TranslateGizmo::setMode(TranslateGizmoMode mode)
{
	this->mode = mode;
	uiMode->setSelectedOptionId(toString(mode));
	sceneEditorWindow.setSetting(EditorSettingType::Editor, "tools.translate.mode", ConfigNode(toString(mode)));
}
