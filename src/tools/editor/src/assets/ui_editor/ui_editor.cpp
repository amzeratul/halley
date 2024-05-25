#include "ui_editor.h"

#include "ui_editor_display.h"
#include "ui_widget_editor.h"
#include "ui_widget_list.h"
#include "halley/tools/project/project.h"
#include "src/scene/choose_window.h"
#include "src/ui/infini_canvas.h"
#include "src/ui/project_window.h"
#include "src/ui/scroll_background.h"
using namespace Halley;

UIEditor::UIEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow, const HalleyAPI& api)
	: AssetEditor(factory, gameResources, project, AssetType::UIDefinition)
	, projectWindow(projectWindow)
	, api(api)
	, undoStack(32)
{
	project.withDLL([&] (ProjectDLL& dll)
	{
		dll.addReloadListener(*this);
	});
}

UIEditor::~UIEditor()
{
	projectWindow.getProject().withDLL([&](ProjectDLL& dll)
	{
		dll.removeReloadListener(*this);
	});
}

void UIEditor::update(Time time, bool moved)
{
	AssetEditor::update(time, moved);

	if (gameI18N) {
		gameI18N->update();
	}

	if (resource) {
		getWidget("saveButton")->setEnabled(isModified());
		getWidget("undoButton")->setEnabled(undoStack.canUndo());
		getWidget("redoButton")->setEnabled(undoStack.canRedo());
	}
}

std::shared_ptr<const Resource> UIEditor::loadResource(const Path& assetPath, const String& id, AssetType assetType)
{
	loadGameFactory();

	uiDefinition = std::make_shared<UIDefinition>(YAMLConvert::parseConfig(project.getAssetsSrcPath() / assetPath));

	if (widgetList) {
		widgetList->setDefinition(uiDefinition);
	}

	factory.loadUI(*this, "halley/ui_editor");

	return uiDefinition;
}

void UIEditor::onMakeUI()
{
	display = getWidgetAs<UIEditorDisplay>("display");
	display->setUIEditor(this);
	widgetList = getWidgetAs<UIWidgetList>("widgetList");
	widgetList->setUIEditor(*this);
	widgetList->setDefinition(uiDefinition);
	widgetEditor = getWidgetAs<UIWidgetEditor>("widgetEditor");
	widgetEditor->setUIEditor(*this, projectWindow);
	widgetEditor->setGameResources(gameResources);

	infiniCanvas = getWidgetAs<InfiniCanvas>("infiniCanvas");
	infiniCanvas->setLeftClickScrollKey(KeyCode::Space);
	infiniCanvas->setZoomListener([=](float zoom)
	{
		display->setZoom(zoom);
	});
	//infiniCanvas->setMouseMirror(display, true);
	display->setCanvas(infiniCanvas);

	setHandle(UIEventType::ListSelectionChanged, "widgetsList", [=] (const UIEvent& event)
	{
		setSelectedWidget(event.getStringData());
	});

	setHandle(UIEventType::ListAccept, "widgetsList", [=] (const UIEvent& event)
	{
		goToWidget(event.getStringData());
	});

	setHandle(UIEventType::ButtonClicked, "addWidget", [=] (const UIEvent& event)
	{
		addWidget();
	});

	setHandle(UIEventType::ButtonClicked, "removeWidget", [=] (const UIEvent& event)
	{
		removeWidget();
	});

	setHandle(UIEventType::ButtonClicked, "replaceWidget", [=] (const UIEvent& event)
	{
		replaceWidget();
	});

	setHandle(UIEventType::ButtonClicked, "saveButton", [=] (const UIEvent& event)
	{
		save();
	});

	setHandle(UIEventType::ButtonClicked, "undoButton", [=] (const UIEvent& event)
	{
		undo();
	});

	setHandle(UIEventType::ButtonClicked, "redoButton", [=] (const UIEvent& event)
	{
		redo();
	});

	setHandle(UIEventType::ButtonClicked, "zoomInButton", [=] (const UIEvent& event)
	{
		infiniCanvas->changeZoom(1, infiniCanvas->getRect().getCenter());
	});

	setHandle(UIEventType::ButtonClicked, "zoomOutButton", [=] (const UIEvent& event)
	{
		infiniCanvas->changeZoom(-1, infiniCanvas->getRect().getCenter());
	});

	setHandle(UIEventType::ButtonClicked, "zoomFitButton", [=] (const UIEvent& event)
	{
		// TODO
	});

	setHandle(UIEventType::ButtonClicked, "centreViewButton", [=] (const UIEvent& event)
	{
		infiniCanvas->setScrollPosition(display->getSize() / 2 * infiniCanvas->getZoomLevel());
	});

	doLoadUI(false);
	reselectWidget();
}

void UIEditor::markModified(bool temporary)
{
	uiDefinition->increaseAssetVersion();
	if (!temporary) {
		modified = true;
		undoStack.update(uiDefinition->getRoot());
	}
}

void UIEditor::onWidgetModified(const String& id, bool temporary)
{
	if (auto data = uiDefinition->findUUID(id); data.result) {
		widgetList->onWidgetModified(id, *data.result);
	}
	markModified(temporary);
}

void UIEditor::reselectWidget()
{
	setSelectedWidget(getWidgetAs<UIList>("widgetsList")->getSelectedOptionId());
}

bool UIEditor::isModified()
{
	return modified;
}

void UIEditor::save()
{
	if (modified) {
		modified = false;

		const auto strData = uiDefinition->toYAML();

		project.setAssetSaveNotification(false);
		project.writeAssetToDisk(assetPath, gsl::as_bytes(gsl::span<const char>(strData.c_str(), strData.length())));
		project.setAssetSaveNotification(true);
	}
}

UIFactory& UIEditor::getGameFactory()
{
	return *gameFactory;
}

bool UIEditor::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::Delete)) {
		removeWidget();
		return true;
	}

	if (key.is(KeyCode::Z, KeyMods::Ctrl)) {
		undo();
		return true;
	}

	if (key.is(KeyCode::Y, KeyMods::Ctrl)) {
		redo();
		return true;
	}

	return false;
}

void UIEditor::onProjectDLLStatusChange(ProjectDLL::Status status)
{
	if (status == ProjectDLL::Status::Unloaded) {
		if (display) {
			display->setUIEditor(nullptr);
		}
		gameFactory = {};
		gameI18N = {};
		loaded = false;
	} else if (status == ProjectDLL::Status::Loaded) {
		loadGameFactory();
		if (display) {
			display->setUIEditor(this);
		}
		doLoadUI(false);
	}
}

void UIEditor::refreshAssets()
{
	doLoadUI(true);
}

void UIEditor::onResourceLoaded()
{
	doLoadUI(false);
}

void UIEditor::doLoadUI(bool force)
{
	if (uiDefinition && display && (force || !loaded)) {
		display->loadDisplay(*uiDefinition);
		layout();
		if (firstLoad) {
			infiniCanvas->setScrollPosition(display->getSize() / 2);
			firstLoad = false;
		}
		if (!loaded) {
			undoStack.loadInitialValue(uiDefinition->getRoot());
		}
		loaded = true;
	}
}

void UIEditor::setSelectedWidget(const String& id)
{
	curSelection = id;
	const auto result = uiDefinition->findUUID(id);
	widgetEditor->setSelectedWidget(id, result.result, result.parent);
	display->setSelectedWidget(id);
}

void UIEditor::goToWidget(const String& id)
{
	infiniCanvas->setScrollPosition(display->getCurWidgetRect().getCenter());
}

void UIEditor::addWidget()
{
	const auto window = std::make_shared<ChooseUIWidgetWindow>(factory, *gameFactory, false, [=] (std::optional<String> result)
	{
		if (result) {
			addWidget(result.value());
		}
	});
	getRoot()->addChild(window);
}

void UIEditor::addWidget(const String& widgetClass)
{
	ConfigNode data = ConfigNode::MapType();
	if (widgetClass == "sizer") {
		data["sizer"] = ConfigNode::MapType();
	} else if (widgetClass == "spacer") {
		data["spacer"] = ConfigNode::MapType();
	} else {
		ConfigNode::MapType widget;
		widget["class"] = widgetClass;
		data["widget"] = std::move(widget);
	}
	data["uuid"] = UUID::generate().toString();

	addWidgetsAt(curSelection, false, Vector<ConfigNode>{{ std::move(data) }});
}

void UIEditor::removeWidget()
{
	deleteWidgets(Vector<String>{ curSelection });
}

void UIEditor::replaceWidget()
{
	if (curSelection.isEmpty()) {
		return;
	}

	const auto& cur = uiDefinition->findUUID(curSelection);
	if (cur.result) {
		auto& curData = *cur.result;
		const bool mustAllowChildren = curData.hasKey("children") && !curData["children"].asSequence().empty();

		const auto window = std::make_shared<ChooseUIWidgetWindow>(factory, *gameFactory, mustAllowChildren, [&] (std::optional<String> result)
		{
			if (result) {
				if (result != "sizer" && result != "spacer") {
					auto oldWidget = std::move(curData["widget"]);
					oldWidget.ensureType(ConfigNodeType::Map);

					ConfigNode::MapType widget;
					widget["id"] = oldWidget["id"];
					widget["size"] = oldWidget["size"];
					widget["enabled"] = oldWidget["enabled"];
					widget["active"] = oldWidget["active"];
					widget["tooltip"] = oldWidget["tooltip"];
					widget["tooltipKey"] = oldWidget["tooltipKey"];
					widget["childLayerAdjustment"] = oldWidget["childLayerAdjustment"];
					widget["class"] = result.value();
					curData["widget"] = widget;
				} else {
					curData.removeKey("widget");
				}

				onWidgetModified(curSelection);
				setSelectedWidget(curSelection);
			}
		});
		getRoot()->addChild(window);
	}
}

void UIEditor::loadGameFactory()
{
	gameI18N = std::make_unique<I18N>(gameResources, I18NLanguage("en-GB"));
	auto* game = project.getGameInstance();
	gameFactory = game->createUIFactory(projectWindow.getAPI(), gameResources, *gameI18N);
	gameFactory->setFallbackFactory(factory);
}

void UIEditor::reassignUUIDs(ConfigNode& node) const
{
	if (node.getType() == ConfigNodeType::Map) {
		node["uuid"] = UUID::generate().toString();
		if (node.hasKey("children")) {
			reassignUUIDs(node["children"]);
		}
	} else if (node.getType() == ConfigNodeType::Sequence) {
		for (auto& n: node) {
			reassignUUIDs(n);
		}
	}
}

void UIEditor::undo()
{
	if (undoStack.canUndo()) {
		uiDefinition->getRoot() = undoStack.undo();
		reloadUI();
	}
}

void UIEditor::redo()
{
	if (undoStack.canRedo()) {
		uiDefinition->getRoot() = undoStack.redo();
		reloadUI();
	}
}

void UIEditor::reloadUI()
{
	display->loadDisplay(*uiDefinition);
	widgetList->setDefinition(uiDefinition);
	layout();
}

void UIEditor::copyWidgets(const Vector<String>& uuids)
{
	ConfigNode::SequenceType result;

	for (const auto& uuid: uuids) {
		const auto widget = uiDefinition->findUUID(uuid);
		if (widget.result) {
			result.push_back(*widget.result);
		}
	}

	YAMLConvert::EmitOptions options;
	options.mapKeyOrder = { "uuid", "widget", "fill", "sizer", "children" };
	const auto str = YAMLConvert::generateYAML(ConfigNode(std::move(result)), options);
	api.system->getClipboard()->setData(str);
}

void UIEditor::cutWidgets(const Vector<String>& uuids)
{
	copyWidgets(uuids);
	deleteWidgets(uuids);
}

void UIEditor::pasteAt(const String& referenceId, bool asChild)
{
	const auto str = api.system->getClipboard()->getStringData();
	if (str) {
		auto config = YAMLConvert::parseConfig(*str);
		if (config.getType() == ConfigNodeType::Sequence) {
			reassignUUIDs(config);

			addWidgetsAt(referenceId, asChild, config.asSequence());
		}
	}
}

void UIEditor::addWidgetsAt(const String& referenceId, bool requestedAsChild, Vector<ConfigNode> datas)
{
	auto result = uiDefinition->findUUID(referenceId);
	if (result.result) {
		auto& referenceNode = *result.result;
		const auto widgetClass = referenceNode.hasKey("widget") ? referenceNode["widget"]["class"].asString("") : "sizer";
		const bool canHaveChildren = widgetClass == "sizer" || gameFactory->getPropertiesForWidget(widgetClass).canHaveChildren;
		const bool canHaveSiblings = result.parent != nullptr;
		if (!canHaveChildren && !canHaveSiblings) {
			// Give up
			return;
		}
		const bool asChild = (requestedAsChild && canHaveChildren) || (!canHaveSiblings);

		auto& parent = asChild ? referenceNode : *result.parent;
		parent["children"].ensureType(ConfigNodeType::Sequence);
		auto& parentChildren = parent["children"].asSequence();
		const auto childIdx = std::min(parentChildren.size(), asChild ? std::numeric_limits<size_t>::max() : size_t(result.childIdx + 1));

		int i = 0;
		for (auto& data: datas) {
			widgetList->addWidget(data, parent["uuid"].asString(), childIdx + i);
			parentChildren.insert(parentChildren.begin() + childIdx + i, std::move(data));
			i++;
		}

		markModified();
	}
}

void UIEditor::deleteWidgets(const Vector<String>& uuids)
{
	bool modified = false;
	for (auto& id : uuids) {
		auto result = uiDefinition->findUUID(id);
		if (result.result && result.parent) {
			auto& parentChildren = (*result.parent)["children"].asSequence();
			std_ex::erase_if(parentChildren, [=](const ConfigNode& n) { return &n == result.result; });
			widgetList->getList().removeItem(id);
			modified = true;
		}
	}
	if (modified) {
		markModified();
	}
}

void UIEditor::selectWidget(const String& id)
{
	widgetList->selectWidget(id);
}


ChooseUIWidgetWindow::ChooseUIWidgetWindow(UIFactory& factory, UIFactory& gameFactory, bool mustAllowChildren, Callback callback)
	: ChooseAssetWindow(Vector2f(), factory, std::move(callback), {})
	, factory(factory)
	, gameFactory(gameFactory)
{
	auto ids = gameFactory.getWidgetClassList(mustAllowChildren);
	std::sort(ids.begin(), ids.end());
	ids.push_back("sizer");
	ids.push_back("spacer");

	setAssetIds(std::move(ids), "widget");
	setTitle(LocalisedString::fromHardcodedString("Choose Widget"));
}

std::shared_ptr<UIImage> ChooseUIWidgetWindow::makeIcon(const String& id, bool hasSearch)
{
	String iconName;
	if (id == "sizer") {
		iconName = "widget_icons/sizer_horizontal.png";
	} else if (id == "spacer") {
		iconName = "widget_icons/spacer.png";
	} else {
		iconName = gameFactory.getPropertiesForWidget(id).iconName;
	}
	auto sprite = iconName.isEmpty() ? Sprite() : Sprite().setImage(gameFactory.getResources(), iconName);
	return std::make_shared<UIImage>(std::move(sprite));
}

LocalisedString ChooseUIWidgetWindow::getItemLabel(const String& id, const String& name, bool hasSearch)
{
	String label;
	if (id == "sizer") {
		label = "Sizer";
	} else if (id == "spacer") {
		label = "Spacer";
	} else {
		label = gameFactory.getPropertiesForWidget(id).name;
	}

	return LocalisedString::fromUserString(label);
}

void ChooseUIWidgetWindow::sortItems(Vector<std::pair<String, String>>& values)
{
}

int ChooseUIWidgetWindow::getNumColumns(Vector2f scrollPaneSize) const
{
	return 3;
}
