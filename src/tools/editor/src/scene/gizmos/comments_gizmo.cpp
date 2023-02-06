#include "comments_gizmo.h"
using namespace Halley;

CommentsGizmo::CommentsGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow)
	: SceneEditorGizmo(snapRules)
{
}

void CommentsGizmo::update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState)
{
}

void CommentsGizmo::draw(Painter& painter, const ISceneEditor& sceneEditor) const
{
}

std::shared_ptr<UIWidget> CommentsGizmo::makeUI()
{
	return {};
}

bool CommentsGizmo::isHighlighted() const
{
	return false;
}

Vector<String> CommentsGizmo::getHighlightedComponents() const
{
	return {};
}

bool CommentsGizmo::onKeyPress(KeyboardKeyPress key)
{
	return false;
}

bool CommentsGizmo::canBoxSelectEntities() const
{
	return false;
}
