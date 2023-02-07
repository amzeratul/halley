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
	forceHighlight = false;

	const auto curVersion = comments.getVersion();
	if (curVersion != lastVersion) {
		lastVersion = curVersion;
		updateHandles();
	}

	bool changed = false;
	std::optional<UUID> highlightedHandle;
	for (auto& handle: handles) {
		const auto deltaPos = handle.update(inputState, handles);
		if (deltaPos) {
			changed = true;
		}

		if (handle.isOver()) {
			highlightedHandle = UUID(handle.getId());
		}
	}

	if (changed) {
		for (auto& handle: handles) {
			if (handle.isHeld()) {
				comments.updateComment(UUID(handle.getId()), [&] (ProjectComment& comment)
				{
					comment.pos = handle.getPosition() + getWorldOffset();
				});
			}
		}		
	}

	if (inputState.mousePos) {
		lastMousePos = *inputState.mousePos;
		if (inputState.rightClickPressed) {
			if (highlightedHandle) {
				editComment(*highlightedHandle);
			} else {
				addComment(*inputState.mousePos);
			}
		}
	}
}

void CommentsGizmo::draw(Painter& painter, const ISceneEditor& sceneEditor) const
{
	const float size = 64;
	const auto rect = Rect4f(-size / 2, -size / 2, size, size);

	for (auto& handle: handles) {
		auto r = rect + handle.getPosition();
		painter.drawRect(r, 2, Colour4f(1, 1, 1, handle.isOver() ? 1.0f : 0.5f));
	}
}

std::shared_ptr<UIWidget> CommentsGizmo::makeUI()
{
	return {};
}

void CommentsGizmo::addComment(Vector2f pos)
{
	auto uuid = comments.addComment(ProjectComment(pos + getWorldOffset(), sceneEditorWindow.getSceneNameForComments()));
	handles.push_back(makeHandle(uuid, pos));
	forceHighlight = true;
}

void CommentsGizmo::editComment(const UUID& uuid)
{

}

SceneEditorGizmoHandle CommentsGizmo::makeHandle(const UUID& uuid, Vector2f pos)
{
	const float size = 64;
	const auto rect = Rect4f(-size / 2, -size / 2, size, size);

	auto handle = SceneEditorGizmoHandle(uuid.toString());
	handle.setPosition(pos, true);
	handle.setBoundsCheck([=] (Vector2f myPos, Vector2f mousePos) -> bool
	{
		return (rect + myPos).contains(mousePos);
	});

	return handle;
}

void CommentsGizmo::updateHandles()
{
	const auto activeComments = comments.getComments(sceneEditorWindow.getSceneNameForComments());
	handles.resize(activeComments.size());
	for (size_t i = 0; i < activeComments.size(); ++i) {
		handles[i] = makeHandle(activeComments[i].first, activeComments[i].second->pos - getWorldOffset());
	}
}

Vector2f CommentsGizmo::getWorldOffset() const
{
	return sceneEditorWindow.getWorldOffset();
}

bool CommentsGizmo::isHighlighted() const
{
	return forceHighlight || std::any_of(handles.begin(), handles.end(), [&](const auto& handle) { return handle.isOver(); });
}

Vector<String> CommentsGizmo::getHighlightedComponents() const
{
	return {};
}

bool CommentsGizmo::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::A, KeyMods::Ctrl)) {
		addComment(lastMousePos);
		return true;
	}

	return false;
}

bool CommentsGizmo::canBoxSelectEntities() const
{
	return false;
}
