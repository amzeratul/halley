#include "scene_editor_window.h"
#include "choose_asset_window.h"
#include "entity_editor.h"
#include "entity_icons.h"
#include "entity_list.h"
#include "halley/entity/entity_factory.h"
#include "halley/entity/prefab_scene_data.h"
#include "halley/entity/world.h"
#include "halley/tools/file/filesystem.h"
#include "halley/tools/project/project.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/ui/ui_factory.h"
#include "scene_editor_canvas.h"
#include "scene_editor_game_bridge.h"
#include "scene_editor_gizmo_collection.h"
#include "halley/tools/project/project_properties.h"
#include "src/ui/project_window.h"
using namespace Halley;

SceneEditorWindow::SceneEditorWindow(UIFactory& factory, Project& project, const HalleyAPI& api, ProjectWindow& projectWindow)
	: UIWidget("scene_editor", {}, UISizer())
	, api(api)
	, uiFactory(factory)
	, project(project)
	, projectWindow(projectWindow)
	, gameBridge(std::make_shared<SceneEditorGameBridge>(api, uiFactory.getResources(), uiFactory, project, projectWindow, *this))
	, entityIcons(std::make_shared<EntityIcons>(project.getGameResources(), *factory.getColourScheme()))
{
	makeUI();

	project.withDLL([&] (DynamicLibrary& dll)
	{
		dll.addReloadListener(*this);
	});
}

SceneEditorWindow::~SceneEditorWindow()
{
	unloadScene();

	project.withDLL([&] (DynamicLibrary& dll)
	{
		dll.removeReloadListener(*this);
	});
}

void SceneEditorWindow::makeUI()
{
	add(uiFactory.makeUI("ui/halley/scene_editor_window"), 1);
	
	canvas = getWidgetAs<SceneEditorCanvas>("canvas");
	canvas->setSceneEditorWindow(*this);
	canvas->setGameBridge(*gameBridge);
	
	entityList = getWidgetAs<EntityList>("entityList");
	entityList->setSceneEditorWindow(*this);

	entityEditor = getWidgetAs<EntityEditor>("entityEditor");
	entityEditor->setSceneEditorWindow(*this);

	toolMode = getWidgetAs<UIList>("toolMode");
	
	setModified(false);

	setHandle(UIEventType::ListSelectionChanged, "entityList_list", [=] (const UIEvent& event)
	{
		onEntitySelected(event.getStringData());
	});

	bindData("toolMode", getSetting(EditorSettingType::Temp, "tools.curTool").asString("drag"), [=] (const String& value)
	{
		if (toolModeTimeout == 0) {
			setTool(value);
			toolModeTimeout = 2;
		}
	});

	setHandle(UIEventType::ListAccept, "entityList_list", [=](const UIEvent& event)
	{
		panCameraToEntity(event.getStringData());
	});

	setHandle(UIEventType::ButtonClicked, "saveButton", [=] (const UIEvent& event)
	{
		saveScene();
	});

	setHandle(UIEventType::ButtonClicked, "undoButton", [=] (const UIEvent& event)
	{
		undo();
	});

	setHandle(UIEventType::ButtonClicked, "redoButton", [=] (const UIEvent& event)
	{
		redo();
	});

	setHandle(UIEventType::ButtonClicked, "addEntity", [=] (const UIEvent& event)
	{
		addNewEntity();
	});

	setHandle(UIEventType::ButtonClicked, "addPrefab", [=] (const UIEvent& event)
	{
		addNewPrefab();
	});

	setHandle(UIEventType::ButtonClicked, "removeEntity", [=] (const UIEvent& event)
	{
		removeEntity();
	});
}

void SceneEditorWindow::onAddedToRoot(UIRoot& root)
{
	root.registerKeyPressListener(shared_from_this());
}

void SceneEditorWindow::loadScene(const String& name)
{
	unloadScene();
	assetPath = project.getImportAssetsDatabase().getPrimaryInputFile(AssetType::Scene, name);

	if (!name.isEmpty()) {
		loadScene(AssetType::Scene, *project.getGameResources().get<Scene>(name));
	}
}

void SceneEditorWindow::loadPrefab(const String& name)
{
	unloadScene();
	assetPath = project.getImportAssetsDatabase().getPrimaryInputFile(AssetType::Prefab, name);

	if (!name.isEmpty()) {
		loadScene(AssetType::Prefab, *project.getGameResources().get<Prefab>(name));
	}
}

void SceneEditorWindow::loadScene(AssetType assetType, const Prefab& origPrefab)
{
	gameBridge->initializeInterfaceIfNeeded();
	if (gameBridge->isLoaded()) {
		auto& interface = gameBridge->getInterface();
		auto& world = interface.getWorld();

		// Load prefab
		prefab = origPrefab.clone();
		origPrefabAssetType = assetType;

		// Spawn scene
		entityFactory = std::make_shared<EntityFactory>(world, project.getGameResources());
		auto sceneCreated = entityFactory->createScene(prefab, true);
		interface.spawnPending();

		// Setup editors
		sceneData = std::make_shared<PrefabSceneData>(*prefab, entityFactory, world, project.getGameResources());
		entityEditorFactory = std::make_shared<EntityEditorFactory>(uiFactory);
		entityEditorFactory->addFieldFactories(interface.getComponentEditorFieldFactories());
		entityEditor->setECSData(project.getECSData());
		entityEditor->setEntityEditorFactory(entityEditorFactory);
		entityList->setSceneData(sceneData);

		// Setup tools
		gameBridge->getGizmos().resetTools();
		interface.setupTools(*toolMode, gameBridge->getGizmos());
		setTool(getSetting(EditorSettingType::Temp, "tools.curTool").asString("translate"));

		// Move camera
		if (!gameBridge->loadCameraPos()) {
			if (!sceneCreated.getEntities().empty()) {
				panCameraToEntity(sceneCreated.getEntities().at(0).getInstanceUUID().toString());
			}
		}
		
		currentEntityScene = sceneCreated;

		// Custom UI
		setCustomUI(gameBridge->makeCustomUI());

		// Console
		setupConsoleCommands();

		// Done
		gameBridge->onSceneLoaded(*prefab);
	}
}

void SceneEditorWindow::unloadScene()
{
	setCustomUI({});

	currentEntityId = "";
	if (gameBridge->isLoaded()) {
		auto& interface = gameBridge->getInterface();
		auto& world = interface.getWorld();
		const auto& cameraIds = interface.getCameraIds();
		for (auto& e: world.getTopLevelEntities()) {
			if (std::find(cameraIds.begin(), cameraIds.end(), e.getEntityId()) == cameraIds.end()) {
				world.destroyEntity(e);
			}
		}
		world.spawnPending();
		gameBridge->unload();
	}
	entityFactory.reset();
	sceneData.reset();
	currentEntityScene.reset();
	entityEditor->unloadEntity();
	entityEditorFactory.reset();
	entityList->setSceneData({});
}

void SceneEditorWindow::update(Time t, bool moved)
{
	if (toolModeTimeout > 0) {
		--toolModeTimeout;
	}

	if (currentEntityScene && entityFactory) {
		if (currentEntityScene->needsUpdate()) {
			currentEntityScene->updateOnEditor(*entityFactory);
			entityList->refreshNames();
		}
	}

	if (buttonsNeedUpdate) {
		buttonsNeedUpdate = false;
		updateButtons();
	}

	if (entityList && gameBridge) {
		gameBridge->setEntityHighlightedOnList(entityList->getEntityUnderCursor());
	}
}

bool SceneEditorWindow::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::S, KeyMods::Ctrl)) {
		saveScene();
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
	
	if (key.is(KeyCode::Delete)) {
		removeEntity(entityList->getCurrentSelection());
		return true;
	}

	if (key.is(KeyCode::C, KeyMods::Ctrl)) {
		copyEntityToClipboard(entityList->getCurrentSelection());
		return true;
	}

	if (key.is(KeyCode::X, KeyMods::Ctrl)) {
		const auto sel = entityList->getCurrentSelection();
		copyEntityToClipboard(sel);
		removeEntity(sel);
		return true;
	}

	if (key.is(KeyCode::V, KeyMods::Ctrl)) {
		pasteEntityFromClipboard(entityList->getCurrentSelection());
		return true;
	}

	if (key.is(KeyCode::D, KeyMods::Ctrl)) {
		duplicateEntity(entityList->getCurrentSelection());
		return true;
	}

	if (key.is(KeyCode::N, KeyMods::Ctrl)) {
		addNewEntity();
		return true;
	}

	if (key.is(KeyCode::N, KeyMods::CtrlShift)) {
		addNewPrefab();
		return true;
	}

	if (key.is(KeyCode::F1)) {
		toggleConsole();
		return true;
	}

	if (gameBridge && toolMode) {
		auto& gizmos = gameBridge->getGizmos();
		if (gizmos.onKeyPress(key, *toolMode)) {
			return true;
		}
	}

	return false;
}

void SceneEditorWindow::onUnloadDLL()
{
	unloadScene();
}

void SceneEditorWindow::onLoadDLL()
{
	if (prefab) {
		loadScene(origPrefabAssetType, *prefab);
	}
}

void SceneEditorWindow::selectEntity(const String& id)
{
	entityList->select(id);
}

void SceneEditorWindow::modifyEntity(const String& id, const EntityDataDelta& delta)
{
	auto& data = sceneData->getWriteableEntityNodeData(id).getData();
	data.applyDelta(delta);
	onEntityModified(id, data, data);
	entityEditor->reloadEntity();
}

void SceneEditorWindow::moveEntity(const String& id, const String& newParent, int childIndex)
{
	auto [prevParent, prevIndex] = sceneData->reparentEntity(id, newParent, childIndex);
	entityList->refreshList();
	onEntityMoved(id, prevParent, static_cast<int>(prevIndex), newParent, childIndex);
}

void SceneEditorWindow::onEntitySelected(const String& id)
{
	String actualId = id;
	if (actualId.isEmpty()) {
		const auto& tree = sceneData->getEntityTree();
		if (tree.entityId.isEmpty()) {
			if (tree.children.empty()) {
				EntityData empty;
				entityEditor->loadEntity("", empty, nullptr, false, project.getGameResources());
				currentEntityId = "";
				return;
			} else {
				actualId = tree.children[0].entityId;
			}
		} else {
			actualId = tree.entityId;
		}
	}

	try {
		auto& entityData = sceneData->getWriteableEntityNodeData(actualId).getData();
		const Prefab* prefabData = nullptr;
		const String prefabName = entityData.getPrefab();
		if (!prefabName.isEmpty()) {
			prefabData = getGamePrefab(prefabName).get();
		}
		
		entityEditor->loadEntity(actualId, entityData, prefabData, false, project.getGameResources());
		gameBridge->setSelectedEntity(UUID(actualId), entityData);
		currentEntityId = actualId;
	} catch (const std::exception& e) {
		Logger::logException(e);
	}
}

void SceneEditorWindow::panCameraToEntity(const String& id)
{
	gameBridge->showEntity(UUID(id));
}

void SceneEditorWindow::saveScene()
{
	clearModifiedFlag();
	undoStack.onSave();

	const auto strData = prefab->toYAML();
	project.writeAssetToDisk(assetPath, gsl::as_bytes(gsl::span<const char>(strData.c_str(), strData.length())));
	gameBridge->onSceneSaved();
}

void SceneEditorWindow::markModified()
{
	setModified(true);
}

void SceneEditorWindow::clearModifiedFlag()
{
	setModified(false);
}

void SceneEditorWindow::onEntityAdded(const String& id, const String& parentId, int childIndex)
{
	const auto& data = sceneData->getEntityNodeData(id).getData();
	entityList->onEntityAdded(id, parentId, childIndex, data);
	sceneData->reloadEntity(parentId.isEmpty() ? id : parentId);
	onEntitySelected(id);

	gameBridge->onEntityAdded(UUID(id), data);

	undoStack.pushAdded(modified, id, parentId, childIndex, data);
	
	markModified();
}

void SceneEditorWindow::onEntityRemoved(const String& id, const String& parentId, int childIndex, const EntityData& prevData)
{
	const String& newSelectionId = getNextSibling(parentId, childIndex);

	undoStack.pushRemoved(modified, id, parentId, childIndex, prevData);
	
	gameBridge->onEntityRemoved(UUID(id));

	entityList->onEntityRemoved(id, newSelectionId);
	sceneData->reloadEntity(parentId.isEmpty() ? id : parentId);
	onEntitySelected(newSelectionId);

	markModified();
}

void SceneEditorWindow::onEntityModified(const String& id, const EntityData& prevData, const EntityData& newData)
{
	if (!id.isEmpty()) {
		const auto& data = sceneData->getEntityNodeData(id).getData();

		const bool hadChange = undoStack.pushModified(modified, id, prevData, newData);

		if (hadChange) {
			entityList->onEntityModified(id, data);
			sceneData->reloadEntity(id);
			gameBridge->onEntityModified(UUID(id), prevData, newData);
			markModified();
		}
	}
}

void SceneEditorWindow::onEntityMoved(const String& id, const String& prevParentId, int prevChildIndex, const String& newParentId, int newChildIndex)
{
	if (currentEntityId == id) {
		onEntitySelected(id);
	}

	gameBridge->onEntityMoved(UUID(id), sceneData->getEntityNodeData(id).getData());

	undoStack.pushMoved(modified, id, prevParentId, prevChildIndex, newParentId, newChildIndex);
	
	markModified();
}

void SceneEditorWindow::onComponentRemoved(const String& name)
{
}

void SceneEditorWindow::onFieldChangedByGizmo(const String& componentName, const String& fieldName)
{
	entityEditor->onFieldChangedByGizmo(componentName, fieldName);
}

void SceneEditorWindow::setTool(String tool)
{
	if (curTool != tool) {
		setTool(tool, "", "");
	}
}

void SceneEditorWindow::setTool(String tool, String componentName, String fieldName)
{
	// This can mutate all parameters
	gameBridge->onToolSet(tool, componentName, fieldName);

	if (tool.isEmpty()) {
		tool = "translate";
	}
	
	curTool = tool;
	curComponentName = componentName;
		
	setToolUI(canvas->setTool(tool, componentName, fieldName));
		
	toolMode->setSelectedOptionId(toString(tool));
}

std::shared_ptr<const Prefab> SceneEditorWindow::getGamePrefab(const String& id) const
{
	if (project.getGameResources().exists<Prefab>(id)) {
		return project.getGameResources().get<Prefab>(id);
	}
	return {};
}

void SceneEditorWindow::copyEntityToClipboard(const String& id)
{
	const auto clipboard = api.system->getClipboard();
	if (clipboard) {
		clipboard->setData(copyEntity(id));
	}
}

void SceneEditorWindow::pasteEntityFromClipboard(const String& referenceId)
{
	const auto clipboard = api.system->getClipboard();
	if (clipboard) {
		auto clipboardData = clipboard->getStringData();
		if (clipboardData) {
			pasteEntity(clipboardData.value(), referenceId);
		}
	}
}

String SceneEditorWindow::copyEntity(const String& id)
{
	return serializeEntity(sceneData->getEntityNodeData(id).getData());
}

void SceneEditorWindow::pasteEntity(const String& stringData, const String& referenceId)
{
	Expects(gameBridge);
	auto data = deserializeEntity(stringData);
	if (data) {
		positionEntityAtCursor(data.value());
		assignUUIDs(data.value());
		addEntity(referenceId, false, std::move(data.value()));
	}
}

void SceneEditorWindow::duplicateEntity(const String& id)
{
	pasteEntity(copyEntity(id), findParent(id));
}

void SceneEditorWindow::openEditPrefabWindow(const String& name)
{
	projectWindow.openAsset(AssetType::Prefab, name);
}

const std::shared_ptr<ISceneData>& SceneEditorWindow::getSceneData() const
{
	return sceneData;
}

void SceneEditorWindow::addNewEntity()
{
	EntityData data;
	data.setInstanceUUID(UUID::generate());
	data.getComponents().emplace_back("Transform2D", ConfigNode::MapType());
	addEntity(std::move(data));
}

void SceneEditorWindow::addNewPrefab()
{
	getRoot()->addChild(std::make_shared<ChooseAssetTypeWindow>(uiFactory, AssetType::Prefab, "", project.getGameResources(), [=] (std::optional<String> result)
	{
		if (result) {
			addNewPrefab(result.value());
		}
	}));
}

void SceneEditorWindow::addNewPrefab(const String& prefabName)
{
	const auto prefab = getGamePrefab(prefabName);
	if (prefab) {
		const auto& entityData = prefab->getEntityData();
		auto components = std::vector<std::pair<String, ConfigNode>>();

		// Clone transform components
		for (const auto& kv: entityData.getComponents()) {
			if (kv.first == "Transform2D" || kv.first == "Transform3D") {
				components.emplace_back(kv);
			}
		}

		EntityData data;
		data.setInstanceUUID(UUID::generate());
		data.setPrefab(prefabName);
		data.setComponents(components);
		addEntity(std::move(data));
	}
}

void SceneEditorWindow::addEntity(EntityData data)
{
	addEntity(currentEntityId, false, std::move(data));
}

void SceneEditorWindow::addEntity(const String& referenceEntity, bool childOfReference, EntityData data)
{
	if (referenceEntity.isEmpty()) {
		addEntity(String(), -1, std::move(data));
	} else {
		positionEntityAtCursor(data);
		
		const bool isScene = prefab->isScene();
		
		const auto& ref = sceneData->getEntityNodeData(referenceEntity);
		const bool canBeSibling = !ref.getParentId().isEmpty() || isScene;
		const bool canBeChild = ref.getData().getPrefab().isEmpty();
		if (!canBeChild && !canBeSibling) {
			return;
		}
		
		const bool addAsChild = (childOfReference && canBeChild) || !canBeSibling;
		
		if (addAsChild) {
			const String& parentId = referenceEntity;
			const int childIndex = -1;
			addEntity(parentId, childIndex, std::move(data));
		} else {
			const String& parentId = ref.getParentId();
			const auto& parentRef = sceneData->getEntityNodeData(parentId);
			const auto idx = parentRef.getData().getChildIndex(UUID(referenceEntity));
			const int childIndex = idx ? static_cast<int>(idx.value() + 1) : -1;
			addEntity(parentId, childIndex, std::move(data));
		}
	}
}

void SceneEditorWindow::addEntity(const String& parentId, int childIndex, EntityData data)
{
	EntityData& parentData = sceneData->getWriteableEntityNodeData(parentId).getData();
	if (parentData.getPrefab().isEmpty() && (parentId != "" || parentData.isSceneRoot())) {
		const auto uuid = data.getInstanceUUID().toString();

		const size_t insertPos = childIndex >= 0 ? static_cast<size_t>(childIndex) : std::numeric_limits<size_t>::max();
		auto& seq = parentData.getChildren();
		seq.insert(seq.begin() + std::min(insertPos, seq.size()), std::move(data));

		onEntityAdded(uuid, parentId, childIndex);
	}
}

void SceneEditorWindow::removeEntity()
{
	if (!currentEntityId.isEmpty()) {
		removeEntity(currentEntityId);
	}
}

void SceneEditorWindow::removeEntity(const String& targetId)
{
	const String& parentId = findParent(currentEntityId);

	auto& data = sceneData->getWriteableEntityNodeData(parentId).getData();
	const bool isSceneRoot = parentId.isEmpty() && data.isSceneRoot();
	if (parentId.isEmpty() && !isSceneRoot) {
		// Don't delete root of prefab
		return;
	}

	auto& children = data.getChildren();
	for (auto iter = children.begin(); iter != children.end(); ++iter) {
		if (iter->getInstanceUUID().toString() == targetId) {
			const auto data = std::move(*iter);
			const int idx = static_cast<int>(iter - children.begin());
			children.erase(iter);
			onEntityRemoved(targetId, parentId, idx, data);
			break;
		}
	}
}

String SceneEditorWindow::findParent(const String& entityId) const
{
	const auto tree = sceneData->getEntityTree();
	const auto res = findParent(entityId, tree, "");
	return res ? *res : "";
}

const String* SceneEditorWindow::findParent(const String& entityId, const EntityTree& tree, const String& prev) const
{
	if (tree.entityId == entityId) {
		return &prev;
	}

	for (auto& c: tree.children) {
		const auto res = findParent(entityId, c, tree.entityId);
		if (res) {
			return res;
		}
	}

	return nullptr;
}

String SceneEditorWindow::getNextSibling(const String& parentId, int childIndex) const
{
	const auto& node = sceneData->getEntityNodeData(parentId);
	const auto& children = node.getData().getChildren();
	if (children.empty()) {
		// No other sibling, return parent
		return parentId;
	} else {
		if (childIndex < static_cast<int>(children.size())) {
			return children[childIndex].getInstanceUUID().toString();
		} else {
			return children.back().getInstanceUUID().toString();
		}
	}
}

void SceneEditorWindow::setCustomUI(std::shared_ptr<UIWidget> ui)
{
	if (curCustomUI) {
		curCustomUI->destroy();
	}
	curCustomUI = ui;
	
	auto customUIField = getWidget("customUI");
	customUIField->setShrinkOnLayout(true);
	customUIField->clear();
	if (ui) {
		customUIField->add(ui, 1);
	}
}

void SceneEditorWindow::setToolUI(std::shared_ptr<UIWidget> ui)
{
	if (curToolUI) {
		curToolUI->destroy();
	}
	curToolUI = ui;

	auto customUIField = canvas->getWidget("currentToolUI");
	customUIField->setShrinkOnLayout(true);
	customUIField->clear();
	if (ui) {
		customUIField->add(ui, 1);
	}
	customUIField->setActive(!!ui);
}

void SceneEditorWindow::setModified(bool enabled)
{
	modified = enabled;
	buttonsNeedUpdate = true;
}

bool SceneEditorWindow::isModified() const
{
	return modified;
}

const EntityIcons& SceneEditorWindow::getEntityIcons() const
{
	return *entityIcons;
}

void SceneEditorWindow::refreshAssets()
{
	if (gameBridge) {
		gameBridge->refreshAssets();
	}
}

void SceneEditorWindow::addComponentToCurrentEntity(const String& componentName)
{
	entityEditor->addComponent(componentName);
}

void SceneEditorWindow::setHighlightedComponents(std::vector<String> componentNames)
{
	entityEditor->setHighlightedComponents(std::move(componentNames));
}

const IEntityEditorFactory& SceneEditorWindow::getEntityEditorFactory() const
{
	return *entityEditorFactory;
}

std::shared_ptr<ScriptNodeTypeCollection> SceneEditorWindow::getScriptNodeTypes()
{
	if (gameBridge) {
		return gameBridge->getScriptNodeTypes();
	}
	return {};
}

String SceneEditorWindow::serializeEntity(const EntityData& node) const
{
	YAMLConvert::EmitOptions options;
	options.mapKeyOrder = {{ "name", "icon", "prefab", "uuid", "components", "children" }};
	return YAMLConvert::generateYAML(node.toConfigNode(false), options);
}

std::optional<EntityData> SceneEditorWindow::deserializeEntity(const String& data) const
{
	ConfigFile file;
	try {
		YAMLConvert::parseConfig(file, gsl::as_bytes(gsl::span<const char>(data.c_str(), data.length())));
		if (!isValidEntityTree(file.getRoot())) {
			return {};
		}
		return EntityData(file.getRoot(), false);
	} catch (...) {
		return {};
	}
}

void SceneEditorWindow::assignUUIDs(EntityData& node)
{
	node.setInstanceUUID(UUID::generate());
	for (auto& child: node.getChildren()) {
		assignUUIDs(child);
	}
}

void SceneEditorWindow::positionEntityAtCursor(EntityData& entityData) const
{
	const auto pos = gameBridge->getMousePos().value_or(gameBridge->getCameraPos());
	positionEntity(entityData, pos.round());
}

void SceneEditorWindow::positionEntity(EntityData& entityData, Vector2f pos) const
{
	for (auto& [componentName, component]: entityData.getComponents()) {
		if (componentName == "Transform2D") {
			component.asMap()["position"] = pos;
		}
	}
}

bool SceneEditorWindow::isValidEntityTree(const ConfigNode& node) const
{
	if (node.getType() != ConfigNodeType::Map) {
		return false;
	}
	for (const auto& [k, v]: node.asMap()) {
		if (k != "name" && k != "uuid" && k != "components" && k != "children" && k != "prefab" && k != "icon") {
			return false;
		}
	}
	if (node.hasKey("children")) {
		for (const auto& child: node["children"].asSequence()) {
			if (!isValidEntityTree(child)) {
				return false;
			}
		}
	}
	
	return true;
}

void SceneEditorWindow::toggleConsole()
{
	auto console = getWidgetAs<UIDebugConsole>("debugConsole");
	const bool newState = !console->isActive();

	if (newState) {
		console->show();
	} else {
		console->hide();
	}
}

void SceneEditorWindow::setupConsoleCommands()
{
	auto controller = getWidgetAs<UIDebugConsole>("debugConsole")->getController();
	controller->clearCommands();
	gameBridge->setupConsoleCommands(*controller, *this);
}

void SceneEditorWindow::updateButtons()
{
	getWidgetAs<UIButton>("saveButton")->setEnabled(modified);
	getWidgetAs<UIButton>("undoButton")->setEnabled(undoStack.canUndo());
	getWidgetAs<UIButton>("redoButton")->setEnabled(undoStack.canRedo());
}

void SceneEditorWindow::undo()
{
	undoStack.undo(*this);
	gameBridge->getGizmos().refreshEntity();
	updateButtons();
}

void SceneEditorWindow::redo()
{
	undoStack.redo(*this);
	gameBridge->getGizmos().refreshEntity();
	updateButtons();
}

const ConfigNode& SceneEditorWindow::getSetting(EditorSettingType type, std::string_view id) const
{
	return projectWindow.getSetting(type, id);
}

void SceneEditorWindow::setSetting(EditorSettingType type, std::string_view id, ConfigNode data)
{
	projectWindow.setSetting(type, id, std::move(data));
}

void SceneEditorWindow::onTabbedIn()
{
	setTool(getSetting(EditorSettingType::Temp, "tools.curTool").asString("translate"));
}

float SceneEditorWindow::getProjectDefaultZoom() const
{
	return project.getProperties().getDefaultZoom();
}

std::shared_ptr<EntityFactory> SceneEditorWindow::getEntityFactory() const
{
	return entityFactory;
}

void SceneEditorWindow::spawnUI(std::shared_ptr<UIWidget> ui)
{
	getRoot()->addChild(std::move(ui));
}

void SceneEditorWindow::openAsset(AssetType assetType, const String& assetId)
{
	projectWindow.openAsset(AssetType::Scene, assetId);
}

String SceneEditorWindow::getAssetKey() const
{
	return origPrefabAssetType + ":" + prefab->getAssetId();
}
