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

	tooltipBg = Sprite()
		.setImage(factory.getResources(), "whitebox.png")
		.setColour(Colour4f(0, 0, 0, 0.5f));

	tooltipText = TextRenderer()
		.setFont(factory.getResources().get<Font>("Ubuntu Bold"))
		.setColour(Colour4f(1, 1, 1))
		.setSize(14);
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

		if (handle.isOver()) {
			const float maxWidth = 200;

			auto t = tooltipText.clone()
				.setSize(14 / zoom)
				.setText(tooltipText.split(comment.text, maxWidth));
			auto tooltipSize = t.getExtents();

			const auto tooltipBorder = Vector2f(5, 5) / zoom;
			const auto tooltipPos = handle.getPosition() + (nodeSize * nodeScale) * Vector2f(0.5f, -0.5f);
			tooltipBg.clone()
				.setPosition(tooltipPos)
				.setSize(tooltipSize + 2 * tooltipBorder)
				.draw(painter);

			t
				.setPosition(tooltipPos + tooltipBorder)
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
	sceneEditorWindow.getUIRoot().addChild(std::make_shared<CommentEditWindow>(factory, comments, uuid));
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



CommentEditWindow::CommentEditWindow(UIFactory& factory, ProjectComments& comments, const UUID& uuid)
	: UIWidget("", {}, UISizer())
	, factory(factory)
	, comments(comments)
	, uuid(uuid)
{
	factory.loadUI(*this, "halley/comment_edit_popup");
	setAnchor(UIAnchor());
}

void CommentEditWindow::onAddedToRoot(UIRoot& root)
{
	root.registerKeyPressListener(shared_from_this(), 10);
	root.focusNext(false);
}

void CommentEditWindow::onRemovedFromRoot(UIRoot& root)
{
	root.removeKeyPressListener(*this);
}

void CommentEditWindow::onMakeUI()
{
	loadComment();

	setHandle(UIEventType::ButtonClicked, "ok", [=](const UIEvent& event)
	{
		onOK();
	});
	
	setHandle(UIEventType::ButtonClicked, "cancel", [=](const UIEvent& event)
	{
		onCancel();
	});
}

bool CommentEditWindow::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::Enter)) {
		onOK();
	}

	if (key.is(KeyCode::Esc)) {
		onCancel();
	}

	return false;
}

void CommentEditWindow::loadComment()
{
	const auto& comment = comments.getComment(uuid);
	getWidgetAs<UIDropdown>("priority")->setSelectedOption(toString(comment.priority));
	getWidgetAs<UITextInput>("comment")->setText(comment.text);
}

void CommentEditWindow::onOK()
{
	comments.updateComment(uuid, [&](ProjectComment& comment) {
		comment.priority = fromString<ProjectCommentPriority>(getWidgetAs<UIDropdown>("priority")->getSelectedOptionId());
		comment.text = getWidgetAs<UITextInput>("comment")->getText();
	}, true);
	destroy();
}

void CommentEditWindow::onCancel()
{
	destroy();
}
