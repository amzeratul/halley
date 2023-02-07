#include "comments_gizmo.h"
#include "halley/tools/project/project.h"
#include "halley/tools/project/project_comments.h"
using namespace Halley;

namespace {
	constexpr static Vector2f commentSize = { 32, 32 };
}

CommentsGizmo::CommentsGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow)
	: SceneEditorGizmo(snapRules)
	, comments(dynamic_cast<Project&>(sceneEditorWindow.getProject()).getComments())
	, factory(factory)
	, sceneEditorWindow(sceneEditorWindow)
{
	commentBg = Sprite()
		.setImage(factory.getResources(), "halley_ui/ui_float_solid_window.png")
		.setPivot(Vector2f(0.5f, 0.5f));

	commentOutline = Sprite()
		.setImage(factory.getResources(), "halley_ui/ui_float_solid_window_outline.png")
		.setPivot(Vector2f(0.5f, 0.5f));

	commentIconNormal = Sprite()
		.setImage(factory.getResources(), "ui/comment_icon_normal.png")
		.setPivot(Vector2f(0.5f, 0.5f));
}

void CommentsGizmo::update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState)
{
	const auto zoom = sceneEditor.getZoom();
	nodeScale = std::min(1.0f, 2.0f / zoom);

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

	if (!inputState.selectionBox && inputState.leftClickReleased && !highlightedHandle) {
		for (auto& handle: handles) {
			handle.setSelected(false);
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
	const auto zoom = sceneEditor.getZoom();
	const auto nodeScale = std::min(1.0f, 2.0f / zoom);
	const auto curZoom = std::max(0.5f, zoom);
	const auto nodeSize = commentSize;
	const auto border = Vector2f(18, 18);

	for (auto& handle: handles) {
		const auto& comment = comments.getComment(UUID(handle.getId()));

		auto col = getCommentColour(comment.priority);
		if (handle.isOver()) {
			col = col.inverseMultiplyLuma(0.7f);
		}

		commentBg.clone()
			.setPosition(handle.getPosition())
			.setColour(col)
			.setSize(commentBg.getSize() / curZoom)
			.scaleTo(nodeSize * nodeScale + border / curZoom)
			.setSliceScale(1.0f / curZoom)
			.draw(painter);

		commentIconNormal.clone()
			.setScale(nodeScale * 0.5f)
			.setPosition(handle.getPosition())
			.draw(painter);

		if (handle.isSelected()) {
			commentOutline.clone()
				.setPosition(handle.getPosition())
				.setSize(commentBg.getSize() / curZoom)
				.scaleTo(nodeSize * nodeScale + border / curZoom)
				.setSliceScale(1.0f / curZoom)
				.draw(painter);
		}
	}
}

Colour4f CommentsGizmo::getCommentColour(ProjectCommentPriority priority) const
{
	switch (priority) {
	case ProjectCommentPriority::Low:
		return Colour4f::fromString("#14C03A");
	case ProjectCommentPriority::Medium:
		return Colour4f::fromString("#ECBC1A");
	case ProjectCommentPriority::High:
		return Colour4f::fromString("#EA3A1C");
	}
	return Colour4f();
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

void CommentsGizmo::deleteComments()
{
	for (auto& handle: handles) {
		if (handle.isSelected()) {
			comments.deleteComment(UUID(handle.getId()));
		}
	}
	std_ex::erase_if(handles, [&](const auto& handle) { return handle.isSelected(); });
}

SceneEditorGizmoHandle CommentsGizmo::makeHandle(const UUID& uuid, Vector2f pos)
{
	const auto rect = Rect4f(-commentSize.x / 2, -commentSize.y / 2, commentSize.x, commentSize.y);

	auto handle = SceneEditorGizmoHandle(uuid.toString());
	handle.setPosition(pos, true);
	handle.setBoundsCheck([=] (Vector2f myPos, Vector2f mousePos) -> bool
	{
		return (rect * nodeScale + myPos).contains(mousePos);
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

bool CommentsGizmo::blockRightClick() const
{
	return isHighlighted();
}

bool CommentsGizmo::allowEntitySpriteSelection() const
{
	return false;
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

	if (key.is(KeyCode::Delete)) {
		deleteComments();
		return true;
	}

	return false;
}

bool CommentsGizmo::canBoxSelectEntities() const
{
	return false;
}
