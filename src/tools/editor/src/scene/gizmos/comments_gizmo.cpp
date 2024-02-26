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
	auto& resources = factory.getResources();

	commentBg = Sprite()
		.setImage(resources, "halley_ui/ui_float_solid_window.png")
		.setPivot(Vector2f(0.5f, 0.5f));

	commentOutline = Sprite()
		.setImage(resources, "halley_ui/ui_float_solid_window_outline.png")
		.setPivot(Vector2f(0.5f, 0.5f));

	commentIcons[ProjectCommentCategory::Misc] = Sprite().setImage(resources, "comments/comment_icon_misc.png").setPivot(Vector2f(0.5f, 0.5f));
	commentIcons[ProjectCommentCategory::Art] = Sprite().setImage(resources, "comments/comment_icon_art.png").setPivot(Vector2f(0.5f, 0.5f));
	commentIcons[ProjectCommentCategory::Implementation] = Sprite().setImage(resources, "comments/comment_icon_implementation.png").setPivot(Vector2f(0.5f, 0.5f));
	commentIcons[ProjectCommentCategory::Music] = Sprite().setImage(resources, "comments/comment_icon_music.png").setPivot(Vector2f(0.5f, 0.5f));
	commentIcons[ProjectCommentCategory::Sound] = Sprite().setImage(resources, "comments/comment_icon_sound.png").setPivot(Vector2f(0.5f, 0.5f));
	commentIcons[ProjectCommentCategory::Writing] = Sprite().setImage(resources, "comments/comment_icon_writing.png").setPivot(Vector2f(0.5f, 0.5f));

	tooltipBg = Sprite()
		.setImage(resources, "whitebox.png")
		.setColour(Colour4f(0, 0, 0, 0.5f));

	tooltipText = TextRenderer()
		.setFont(resources.get<Font>("Ubuntu Bold"))
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
		if (inputState.rightClickPressed && highlightedHandle) {
			editComment(*highlightedHandle);
		}
		if (inputState.leftClickPressed && inputState.ctrlHeld && !highlightedHandle) {
			addComment(*inputState.mousePos, false);
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

		commentIcons.at(comment.category).clone()
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
	case ProjectCommentPriority::Note:
		return Colour4f::fromString("#3D4988");
	case ProjectCommentPriority::Low:
		return Colour4f::fromString("#14C03A");
	case ProjectCommentPriority::Medium:
		return Colour4f::fromString("#ECBC1A");
	case ProjectCommentPriority::High:
		return Colour4f::fromString("#EA3A1C");
	}
	return Colour4f();
}

void CommentsGizmo::setFilter(std::optional<ProjectCommentCategory> categoryFilter)
{
	this->categoryFilter = categoryFilter;
	updateHandles();
}

std::shared_ptr<UIWidget> CommentsGizmo::makeUI()
{
	auto ui = factory.makeUI("halley/comment_gizmo_toolbar");
	ui->setInteractWithMouse(true);
	
	ui->setHandle(UIEventType::ButtonClicked, "addComment", [=] (const UIEvent& event)
	{
		Concurrent::execute(Executors::getMainUpdateThread(), [=] ()
		{
			addComment(sceneEditorWindow.getWorldCameraPosition(), true);
		});
	});

	ui->setHandle(UIEventType::ButtonClicked, "export", [=] (const UIEvent& event)
	{
		exportComments();
	});

	ui->bindData("categoryFilter", "", [=](String categoryFilter)
	{
		if (categoryFilter == "all") {
			setFilter(std::nullopt);
		} else {
			setFilter(fromString<ProjectCommentCategory>(categoryFilter));
		}
	});
	
	return ui;
}

void CommentsGizmo::addComment(Vector2f pos, bool isWorldSpace)
{
	const auto worldPos = pos + (isWorldSpace ? Vector2f() : getWorldOffset());
	const auto screenPos = pos - (isWorldSpace ? getWorldOffset() : Vector2f());

	const auto category = sceneEditorWindow.getSetting(EditorSettingType::Project, "comments.last_category").asEnum(ProjectCommentCategory::Misc);
	const auto uuid = comments.addComment(ProjectComment(worldPos, sceneEditorWindow.getSceneNameForComments(), category));

	for (auto& handle: handles) {
		handle.setSelected(false);
	}
	handles.push_back(makeHandle(uuid, screenPos));
	//handles.back().setSelected(true);
	forceHighlight = true;

	editComment(uuid, [=] (bool ok)
	{
		if (ok) {
			sceneEditorWindow.setSetting(EditorSettingType::Project, "comments.last_category", ConfigNode(toString(comments.getComment(uuid).category)));
		} else {
			deleteComment(uuid);
		}
	});
}

void CommentsGizmo::editComment(const UUID& uuid, std::function<void(bool)> callback)
{
	sceneEditorWindow.getUIRoot().addChild(std::make_shared<CommentEditWindow>(factory, comments, uuid, std::move(callback)));
}

void CommentsGizmo::deleteComment(const UUID& uuid)
{
	comments.deleteComment(uuid);
	const auto id = uuid.toString();
	std_ex::erase_if(handles, [&](const auto& handle) { return handle.getId() == id; });
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

void CommentsGizmo::exportComments()
{
	FileChooserParameters params;
	params.save = true;
	params.fileTypes.emplace_back(FileChooserParameters::FileType{ "YAML", {"yaml"}, true });
	OS::get().openFileChooser(params).then([this] (std::optional<Path> path)
	{
		if (path) {
			comments.exportAll(categoryFilter, *path);
		}
	});
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
	handles.clear();
	handles.reserve(activeComments.size());
	for (const auto& comment: activeComments) {
		if (!categoryFilter || comment.second->category == categoryFilter) {
			handles.push_back(makeHandle(comment.first, comment.second->pos - getWorldOffset()));
		}
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
		addComment(lastMousePos, false);
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

bool CommentsGizmo::canSelectEntities() const
{
	return false;
}


CommentEditWindow::CommentEditWindow(UIFactory& factory, ProjectComments& comments, const UUID& uuid, Callback callback)
	: PopupWindow("commentEdit")
	, factory(factory)
	, comments(comments)
	, uuid(uuid)
	, callback(std::move(callback))
{
	factory.loadUI(*this, "halley/comment_edit_popup");
}

void CommentEditWindow::onAddedToRoot(UIRoot& root)
{
	root.registerKeyPressListener(shared_from_this(), 10);
	root.focusNext(false);
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

	setHandle(UIEventType::TextSubmit, "comment", [=](const UIEvent& event)
	{
		return onOK();
	});
}

bool CommentEditWindow::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::Enter)) {
		onOK();
		return true;
	}

	if (key.is(KeyCode::Esc)) {
		onCancel();
		return true;
	}

	// Block everything else
	return true;
}

void CommentEditWindow::loadComment()
{
	const auto& comment = comments.getComment(uuid);
	getWidgetAs<UIDropdown>("priority")->setSelectedOption(toString(comment.priority));
	getWidgetAs<UIDropdown>("category")->setSelectedOption(toString(comment.category));
	getWidgetAs<UITextInput>("comment")->setText(comment.text);
}

void CommentEditWindow::onOK()
{
	comments.updateComment(uuid, [&](ProjectComment& comment) {
		comment.priority = fromString<ProjectCommentPriority>(getWidgetAs<UIDropdown>("priority")->getSelectedOptionId());
		comment.category = fromString<ProjectCommentCategory>(getWidgetAs<UIDropdown>("category")->getSelectedOptionId());
		comment.text = getWidgetAs<UITextInput>("comment")->getText();
	}, true);
	if (callback) {
		callback(true);
	}
	destroy();
}

void CommentEditWindow::onCancel()
{
	if (callback) {
		callback(false);
	}
	destroy();
}
