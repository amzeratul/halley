#include "comments_gizmo.h"
#include "halley/tools/project/project.h"
#include "halley/tools/project/project_comments.h"
using namespace Halley;

CommentsGizmo::CommentsGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow)
	: SceneEditorGizmo(snapRules)
	, comments(dynamic_cast<Project&>(sceneEditorWindow.getProject()).getComments())
	, factory(factory)
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
