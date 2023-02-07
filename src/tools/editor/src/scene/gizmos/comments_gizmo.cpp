#include "comments_gizmo.h"
#include "halley/tools/project/project.h"
#include "halley/tools/project/project_comments.h"
using namespace Halley;

CommentsGizmo::CommentsGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow)
	: SceneEditorGizmo(snapRules)
	, comments(dynamic_cast<Project&>(sceneEditorWindow.getProject()).getComments())
	, factory(factory)
	, sceneEditorWindow(sceneEditorWindow)
{
}

void CommentsGizmo::update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState)
{
	const auto curVersion = comments.getVersion();
	if (curVersion != lastVersion) {
		lastVersion = curVersion;
		updateGizmos();
	}

	for (auto& handle: handles) {
		const auto newPos = handle.update(inputState, handles);

		if (newPos) {
			comments.updateComment(UUID(handle.getId()), [&] (ProjectComment& comment)
			{
				comment.pos = *newPos;
			});
		}
	}
}

void CommentsGizmo::draw(Painter& painter, const ISceneEditor& sceneEditor) const
{
	const float size = 64;
	const auto rect = Rect4f(-size / 2, -size / 2, size, size);

	for (auto& handle: handles) {
		auto r = rect + handle.getPosition();
		painter.drawRect(r, 2, Colour4f(1, 1, 1));
	}
}

std::shared_ptr<UIWidget> CommentsGizmo::makeUI()
{
	return {};
}

void CommentsGizmo::updateGizmos()
{
	const float size = 64;
	const auto rect = Rect4f(-size / 2, -size / 2, size, size);

	const auto activeComments = comments.getComments(sceneEditorWindow.getSceneNameForComments());
	handles.resize(activeComments.size());
	for (size_t i = 0; i < activeComments.size(); ++i) {
		auto handle = SceneEditorGizmoHandle(activeComments[i].first.toString());
		handle.setPosition(activeComments[i].second->pos, true);
		handle.setBoundsCheck([=] (Vector2f myPos, Vector2f mousePos) -> bool
		{
			return (rect + myPos).contains(mousePos);
		});

		handles[i] = std::move(handle);
	}
}

bool CommentsGizmo::isHighlighted() const
{
	return std::any_of(handles.begin(), handles.end(), [&](const auto& handle) { return handle.isOver(); });
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
