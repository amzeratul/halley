#include "scene_editor_window.h"
#include "choose_asset_window.h"
#include "entity_editor.h"
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
#include "scene_editor_tabs.h"
using namespace Halley;

SceneEditorWindow::SceneEditorWindow(UIFactory& factory, Project& project, const HalleyAPI& api, SceneEditorTabs& sceneEditorTabs)
	: UIWidget("scene_editor", {}, UISizer())
	, api(api)
	, uiFactory(factory)
	, project(project)
	, sceneEditorTabs(sceneEditorTabs)
	, gameBridge(std::make_shared<SceneEditorGameBridge>(api, uiFactory.getResources(), uiFactory, project))
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
	
	setSaveEnabled(false);

	setHandle(UIEventType::ListSelectionChanged, "entityList_list", [=] (const UIEvent& event)
	{
		onEntitySelected(event.getStringData());
	});

	setHandle(UIEventType::ListSelectionChanged, "toolMode", [=] (const UIEvent& event)
	{
		if (toolModeTimeout == 0) {
			setTool(fromString<SceneEditorTool>(event.getStringData()));
			toolModeTimeout = 2;
		}
	});

	setHandle(UIEventType::ListAccept, "entityList_list", [=](const UIEvent& event)
	{
		panCameraToEntity(event.getStringData());
	});

	setHandle(UIEventType::ButtonClicked, "saveButton", [=] (const UIEvent& event)
	{
		saveEntity();
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

void SceneEditorWindow::onAddedToRoot()
{
	getRoot()->registerKeyPressListener(shared_from_this());
}

void SceneEditorWindow::loadScene(const String& name)
{
	unloadScene();
	assetPath = project.getAssetsSrcPath() / project.getImportAssetsDatabase().getPrimaryInputFile(AssetType::Scene, name);

	if (!name.isEmpty()) {
		loadScene(AssetType::Scene, *project.getGameResources().get<Scene>(name));
	}
}

void SceneEditorWindow::loadPrefab(const String& name)
{
	unloadScene();
	assetPath = project.getAssetsSrcPath() / project.getImportAssetsDatabase().getPrimaryInputFile(AssetType::Prefab, name);

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
		prefab = std::make_shared<Prefab>(origPrefab);
		origPrefabAssetType = assetType;
		preparePrefab(*prefab);

		// Spawn scene
		entityFactory = std::make_shared<EntityFactory>(world, project.getGameResources());
		auto sceneCreated = entityFactory->createScene(prefab, true);
		interface.spawnPending();

		// Setup editors
		sceneData = std::make_shared<PrefabSceneData>(*prefab, entityFactory, world, project.getGameResources());
		entityEditor->setECSData(project.getECSData());
		entityEditor->addFieldFactories(interface.getComponentEditorFieldFactories());
		entityList->setSceneData(sceneData);

		setTool(SceneEditorTool::Translate);

		// Show root
		if (!sceneCreated.getEntities().empty()) {
			panCameraToEntity(sceneCreated.getEntities().at(0).getInstanceUUID().toString());
		}
		currentEntityScene = sceneCreated;

		// Custom UI
		setCustomUI(gameBridge->makeCustomUI());

		// Console
		setupConsoleCommands();

		// Done
		gameBridge->onSceneLoaded(assetType, origPrefab.getAssetId());
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
	entityEditor->resetFieldFactories();
}

void SceneEditorWindow::update(Time t, bool moved)
{
	if (toolModeTimeout > 0) {
		--toolModeTimeout;
	}

	if (currentEntityScene && entityFactory) {
		if (currentEntityScene->needsUpdate()) {
			currentEntityScene->updateOnEditor(*entityFactory);
		}
	}
}

bool SceneEditorWindow::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::S, KeyMods::Ctrl)) {
		saveEntity();
		return true;
	}

	if (key.is(KeyCode::F1)) {
		toggleConsole();
		return true;
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

void SceneEditorWindow::selectEntity(const std::vector<UUID>& candidates)
{
	const auto tree = sceneData->getEntityTree();
	for (auto& c: candidates) {
		const auto found = tree.contains(c.toString());
		if (found) {
			entityList->select(c.toString());
			break;
		}
	}
	entityList->select("");
}

void SceneEditorWindow::onEntitySelected(const String& id)
{
	decayTool();
	
	String actualId = id;
	if (actualId.isEmpty()) {
		const auto& tree = sceneData->getEntityTree();
		if (tree.entityId.isEmpty()) {
			if (tree.children.empty()) {
				ConfigNode empty;
				entityEditor->loadEntity("", empty, nullptr, false, project.getGameResources());
				return;
			} else {
				actualId = tree.children[0].entityId;
			}
		} else {
			actualId = tree.entityId;
		}
	}

	auto& entityData = sceneData->getEntityNodeData(actualId).data;
	const ConfigNode* prefabData = nullptr;
	const String prefabName = entityData["prefab"].asString("");
	if (!prefabName.isEmpty()) {
		prefabData = &getGamePrefab(prefabName)->getRoot();
	}
	
	entityEditor->loadEntity(actualId, entityData, prefabData, false, project.getGameResources());
	gameBridge->setSelectedEntity(UUID(actualId), entityData);
	currentEntityId = actualId;
}

void SceneEditorWindow::panCameraToEntity(const String& id)
{
	gameBridge->showEntity(UUID(id));
}

void SceneEditorWindow::saveEntity()
{
	setSaveEnabled(false);

	const auto strData = prefab->toYAML();
	auto data = gsl::as_bytes(gsl::span<const char>(strData.c_str(), strData.length()));
	FileSystem::writeFile(assetPath, data);
	project.notifyAssetFileModified(assetPath);
}

void SceneEditorWindow::markModified()
{
	setSaveEnabled(true);
}

void SceneEditorWindow::onEntityAdded(const String& id, const String& parentId, const String& afterSiblingId)
{
	auto& data = sceneData->getEntityNodeData(id).data;
	entityList->onEntityAdded(id, parentId, afterSiblingId, data);
	sceneData->reloadEntity(parentId.isEmpty() ? id : parentId);
	onEntitySelected(id);

	gameBridge->onEntityAdded(UUID(id), data);
	
	markModified();
}

void SceneEditorWindow::onEntityRemoved(const String& id, const String& parentId)
{
	gameBridge->onEntityRemoved(UUID(id));

	entityList->onEntityRemoved(id, parentId);
	sceneData->reloadEntity(parentId.isEmpty() ? id : parentId);
	onEntitySelected(parentId);

	markModified();
}

void SceneEditorWindow::onEntityModified(const String& id)
{
	if (!id.isEmpty()) {
		const auto& data = sceneData->getEntityNodeData(id).data;

		entityList->onEntityModified(id, data);

		sceneData->reloadEntity(id);
		
		gameBridge->onEntityModified(UUID(id), data);
	}

	markModified();
}

void SceneEditorWindow::onEntityMoved(const String& id)
{
	if (currentEntityId == id) {
		onEntitySelected(id);
	}

	gameBridge->onEntityMoved(UUID(id), sceneData->getEntityNodeData(id).data);
	
	markModified();
}

void SceneEditorWindow::onComponentRemoved(const String& name)
{
	if (name == curComponentName) {
		decayTool();
	}
}

void SceneEditorWindow::onFieldChangedByGizmo(const String& componentName, const String& fieldName)
{
	entityEditor->onFieldChangedByGizmo(componentName, fieldName);
	onEntityModified(currentEntityId);
}

void SceneEditorWindow::setTool(SceneEditorTool tool)
{
	if (curTool != tool) {
		setTool(tool, "", "", ConfigNode());
	}
}

void SceneEditorWindow::setTool(SceneEditorTool tool, const String& componentName, const String& fieldName, ConfigNode options)
{
	options = gameBridge->onToolSet(tool, componentName, fieldName, std::move(options));

	curTool = tool;
	curComponentName = componentName;
	
	setToolUI(canvas->setTool(tool, componentName, fieldName, options));
	
	toolMode->setItemActive("polygon", tool == SceneEditorTool::Polygon);
	toolMode->setItemActive("vertex", tool == SceneEditorTool::Vertex);
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
	const auto entityData = sceneData->getEntityNodeData(id);
	return serializeEntity(entityData.data);
}

void SceneEditorWindow::pasteEntity(const String& stringData, const String& referenceId)
{
	auto data = deserializeEntity(stringData);
	if (data) {
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
	sceneEditorTabs.load(AssetType::Prefab, name);
}

const std::shared_ptr<ISceneData>& SceneEditorWindow::getSceneData() const
{
	return sceneData;
}

void SceneEditorWindow::addNewEntity()
{
	auto data = ConfigNode(ConfigNode::MapType());
	data["name"] = "";
	data["uuid"] = UUID::generate().toString();
	data["children"] = ConfigNode::SequenceType();
	data["components"] = ConfigNode::SequenceType();
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
		const auto& prefabRoot = prefab->getRoot();
		if (prefabRoot.getType() == ConfigNodeType::Map) {
			auto components = ConfigNode::SequenceType();

			// Clone transform components
			for (auto& c: prefabRoot["components"].asSequence()) {
				for (auto& kv: c.asMap()) {
					if (kv.first == "Transform2D" || kv.first == "Transform3D") {
						components.emplace_back(c);
					}
				}
			}

			auto data = ConfigNode(ConfigNode::MapType());
			data["uuid"] = UUID::generate().toString();
			data["components"] = std::move(components);
			data["prefab"] = prefabName;
			addEntity(std::move(data));
		}
	}
}

void SceneEditorWindow::addEntity(ConfigNode data)
{
	addEntity(currentEntityId, false, std::move(data));
}

void SceneEditorWindow::addEntity(const String& referenceEntity, bool childOfReference, ConfigNode data)
{
	if (referenceEntity.isEmpty()) {
		addEntity(referenceEntity, true, std::move(data));
	} else {
		const bool isScene = sceneData->getEntityNodeData("").data.getType() == ConfigNodeType::Sequence;
		
		const auto ref = sceneData->getEntityNodeData(referenceEntity);
		const bool canBeSibling = !ref.parentId.isEmpty() || isScene;
		const bool canBeChild = !ref.data.hasKey("prefab");
		if (!canBeChild && !canBeSibling) {
			return;
		}
		
		const bool addAsChild = (childOfReference && canBeChild) || !canBeSibling;
		const String& parentId = addAsChild ? referenceEntity : ref.parentId;
		const String& siblingId = addAsChild ? "" : referenceEntity;

		addEntity(parentId, siblingId, std::move(data));
	}
}

void SceneEditorWindow::addEntity(const String& parentId, const String& afterSibling, ConfigNode data)
{
	auto addOnSequence = [&] (ConfigNode::SequenceType& seq)
	{
		const auto uuid = data["uuid"].asString();
		auto insertPos = std::find_if(seq.begin(), seq.end(), [&] (const ConfigNode& node) -> bool
		{
			return node["uuid"].asString() == afterSibling;
		});		
		if (insertPos != seq.end()) {
			++insertPos;
		}
		seq.insert(insertPos, std::move(data));
		onEntityAdded(uuid, parentId, afterSibling);
	};
	
	if (parentId.isEmpty()) {
		// Should only be able to place items on the root if it's a scene, and therefore the root is a sequence
		auto& root = sceneData->getEntityNodeData("").data;
		if (root.getType() == ConfigNodeType::Sequence) {
			addOnSequence(root.asSequence());
		}
	} else {
		ConfigNode& parentData = sceneData->getEntityNodeData(parentId).data;
		if (!parentData.hasKey("prefab")) {
			auto& children = parentData["children"];
			children.ensureType(ConfigNodeType::Sequence);
			addOnSequence(children.asSequence());
		}
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

	auto& data = sceneData->getEntityNodeData(parentId).data;
	const bool isSceneRoot = parentId.isEmpty() && data.getType() == ConfigNodeType::Sequence;
	if (parentId.isEmpty() && !isSceneRoot) {
		// Don't delete root of prefab
		return;
	}

	auto& children = isSceneRoot ? data.asSequence() : data["children"].asSequence();
	children.erase(std::remove_if(children.begin(), children.end(), [&] (const ConfigNode& child)
	{
		return child["uuid"].asString("") == targetId;
	}), children.end());

	onEntityRemoved(targetId, parentId);
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

void SceneEditorWindow::preparePrefab(Prefab& prefab)
{
	auto& root = prefab.getRoot();
	if (root.getType() == ConfigNodeType::Sequence) {
		for (auto& rootElement: root.asSequence()) {
			preparePrefabEntity(rootElement);
		}
	} else {
		preparePrefabEntity(prefab.getRoot());
	}
}

void SceneEditorWindow::preparePrefabEntity(ConfigNode& node)
{
	const bool isPrefab = node.hasKey("prefab");
	
	if (!node.hasKey("uuid")) {
		node["uuid"] = UUID::generate().toString();
	}

	if (!node.hasKey("components")) {
		node["components"] = ConfigNode::SequenceType();
	}

	if (!isPrefab) {
		if (!node.hasKey("name")) {
			node["name"] = "Entity";
		}

		if (!node.hasKey("children")) {
			node["children"] = ConfigNode::SequenceType();
		} else {
			for (auto& c: node["children"].asSequence()) {
				preparePrefabEntity(c);
			}
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

void SceneEditorWindow::decayTool()
{
	if (curTool == SceneEditorTool::Polygon) {
		setTool(SceneEditorTool::Translate);
	}
}

void SceneEditorWindow::setSaveEnabled(bool enabled)
{
	auto button = getWidgetAs<UIButton>("saveButton");
	button->setLabel(LocalisedString::fromHardcodedString(enabled ? "* Save" : "Save"));
	button->setEnabled(enabled);
}

String SceneEditorWindow::serializeEntity(const ConfigNode& node) const
{
	YAMLConvert::EmitOptions options;
	options.mapKeyOrder = {{ "name", "uuid", "components", "children" }};
	return YAMLConvert::generateYAML(node, options);
}

std::optional<ConfigNode> SceneEditorWindow::deserializeEntity(const String& data) const
{
	ConfigFile file;
	try {
		YAMLConvert::parseConfig(file, gsl::as_bytes(gsl::span<const char>(data.c_str(), data.length())));
		if (!isValidEntityTree(file.getRoot())) {
			return {};
		}
		return std::move(file.getRoot());
	} catch (...) {
		return {};
	}
}

void SceneEditorWindow::assignUUIDs(ConfigNode& node)
{
	node["uuid"] = UUID::generate().toString();
	if (node["children"].getType() == ConfigNodeType::Sequence) {
		for (auto& child: node["children"].asSequence()) {
			assignUUIDs(child);
		}
	}
}

bool SceneEditorWindow::isValidEntityTree(const ConfigNode& node) const
{
	if (node.getType() != ConfigNodeType::Map) {
		return false;
	}
	for (const auto& [k, v]: node.asMap()) {
		if (k != "name" && k != "uuid" && k != "components" && k != "children" && k != "prefab") {
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
