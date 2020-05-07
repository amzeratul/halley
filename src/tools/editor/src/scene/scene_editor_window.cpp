#include "scene_editor_window.h"
#include "choose_asset_window.h"
#include "entity_editor.h"
#include "entity_list.h"
#include "halley/entity/entity_factory.h"
#include "halley/entity/prefab_scene_data.h"
#include "halley/entity/world.h"
#include "halley/tools/file/filesystem.h"
#include "halley/tools/project/project.h"
#include "halley/tools/yaml/yaml_convert.h"
#include "halley/ui/ui_factory.h"
#include "scene_editor_canvas.h"
#include "scene_editor_game_bridge.h"
using namespace Halley;

SceneEditorWindow::SceneEditorWindow(UIFactory& factory, Project& project, const HalleyAPI& api)
	: UIWidget("scene_editor", {}, UISizer())
	, api(api)
	, uiFactory(factory)
	, project(project)
	, gameBridge(std::make_shared<SceneEditorGameBridge>(api, uiFactory.getResources(), uiFactory, project))
{
	makeUI();
}

SceneEditorWindow::~SceneEditorWindow()
{
	unloadScene();
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
		selectEntity(event.getStringData());
	});

	setHandle(UIEventType::ListSelectionChanged, "toolMode", [=] (const UIEvent& event)
	{
		setTool(fromString<SceneEditorTool>(event.getStringData()));
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
		loadScene(*project.getGameResources().get<Scene>(name));
	}
}

void SceneEditorWindow::loadPrefab(const String& name)
{
	unloadScene();
	assetPath = project.getAssetsSrcPath() / project.getImportAssetsDatabase().getPrimaryInputFile(AssetType::Prefab, name);

	if (!name.isEmpty()) {
		loadScene(*project.getGameResources().get<Prefab>(name));
	}
}

void SceneEditorWindow::loadScene(const Prefab& origPrefab)
{
	gameBridge->initializeInterfaceIfNeeded();
	if (gameBridge->isLoaded()) {
		auto& interface = gameBridge->getInterface();
		auto& world = interface.getWorld();

		// Load prefab
		prefab = std::make_shared<Prefab>(origPrefab);
		preparePrefab(*prefab);

		// Spawn scene
		entityFactory = std::make_shared<EntityFactory>(world, project.getGameResources());
		auto sceneCreated = entityFactory->createScene(prefab);
		interface.spawnPending();

		// Setup editors
		sceneData = std::make_shared<PrefabSceneData>(*prefab, entityFactory, world, project.getGameResources());
		entityEditor->setECSData(project.getECSData());
		entityEditor->addFieldFactories(interface.getComponentEditorFieldFactories());
		entityList->setSceneData(sceneData);

		setTool(SceneEditorTool::Drag);

		// Show root
		if (!sceneCreated.getEntities().empty()) {
			panCameraToEntity(sceneCreated.getEntities().at(0).getUUID().toString());
		}

		// Custom UI
		setCustomUI(gameBridge->makeCustomUI());
	}
}

void SceneEditorWindow::unloadScene()
{
	setCustomUI({});

	currentEntityId = "";
	if (gameBridge->isLoaded()) {
		auto& interface = gameBridge->getInterface();
		auto& world = interface.getWorld();
		const auto cameraId = interface.getCameraId();
		for (auto& e: world.getTopLevelEntities()) {
			if (e.getEntityId() != cameraId) {
				world.destroyEntity(e);
			}
		}		
		world.spawnPending();
	}
	entityFactory.reset();
	sceneData.reset();
	entityEditor->unloadEntity();
}

void SceneEditorWindow::update(Time t, bool moved)
{
	if (gameBridge->needsReload()) {
		unloadScene();
		gameBridge->reload();
		loadScene(*prefab);
	}
}

bool SceneEditorWindow::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::S, KeyMods::Ctrl)) {
		saveEntity();
		return true;
	}

	return false;
}

void SceneEditorWindow::selectEntity(const String& id)
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

	auto& entityData = sceneData->getEntityData(actualId).data;
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

	YAMLConvert::EmitOptions options;
	options.mapKeyOrder = {{ "name", "uuid", "components", "children" }};

	auto strData = YAMLConvert::generateYAML(*prefab, options);
	auto data = gsl::as_bytes(gsl::span<const char>(strData.c_str(), strData.length()));
	FileSystem::writeFile(assetPath, data);
}

void SceneEditorWindow::markModified()
{
	setSaveEnabled(true);
}

void SceneEditorWindow::onEntityAdded(const String& id, const String& parentId)
{
	auto& data = sceneData->getEntityData(id).data;
	entityList->onEntityAdded(id, parentId, data);
	sceneData->reloadEntity(parentId.isEmpty() ? id : parentId);
	selectEntity(id);

	gameBridge->onEntityAdded(UUID(id), data);
	
	markModified();
}

void SceneEditorWindow::onEntityRemoved(const String& id, const String& parentId)
{
	gameBridge->onEntityRemoved(UUID(id));

	entityList->onEntityRemoved(id, parentId);
	sceneData->reloadEntity(parentId.isEmpty() ? id : parentId);
	selectEntity(parentId);

	markModified();
}

void SceneEditorWindow::onEntityModified(const String& id)
{
	if (!id.isEmpty()) {
		const auto& data = sceneData->getEntityData(id).data;

		entityList->onEntityModified(id, data);

		sceneData->reloadEntity(id);
		
		gameBridge->onEntityModified(UUID(id), data);
	}

	markModified();
}

void SceneEditorWindow::onEntityMoved(const String& id)
{
	if (currentEntityId == id) {
		selectEntity(id);
	}

	gameBridge->onEntityMoved(UUID(id), sceneData->getEntityData(id).data);
	
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
	String parentId;
	if (!currentEntityId.isEmpty()) {
		const String& tryParentId = currentEntityId;
		const auto res = sceneData->getEntityData(tryParentId);
		const bool isParentPrefab = res.data.hasKey("prefab");
		parentId = isParentPrefab ? res.parentId : tryParentId;
	}

	if (!parentId.isEmpty() || prefab->getRoot().getType() == ConfigNodeType::Sequence) {
		addEntity(parentId, std::move(data));
	}
}

void SceneEditorWindow::addEntity(const String& parentId, ConfigNode data)
{
	const auto uuid = data["uuid"].asString();
	
	if (parentId.isEmpty()) {
		// Should only be able to place items on the root if it's a scene
		auto& root = sceneData->getEntityData("").data;
		root.asSequence().emplace_back(std::move(data));
	} else {
		ConfigNode& parentData = sceneData->getEntityData(parentId).data;
		auto& children = parentData["children"];
		if (children.getType() != ConfigNodeType::Sequence) {
			children = ConfigNode::SequenceType();
		}
		children.asSequence().emplace_back(std::move(data));
	}

	onEntityAdded(uuid, parentId);
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

	auto& data = sceneData->getEntityData(parentId).data;
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

static void hackFixPosition(Vector2f offset, ConfigNode& node)
{
	if (node.hasKey("components")) {
		for (auto& c: node["components"]) {
			for (auto& [name, value]: c.asMap()) {
				for (auto& [fieldName, fieldValue]: value.asMap()) {
					if (fieldName == "baseline" || fieldName == "polygon") {
						for (auto& v: fieldValue.asSequence()) {
							v = v.asVector2f() + offset;
						}
					}
				}
			}
		}
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

	//hackFixPosition(Vector2f(-320, -240), node);
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
		setTool(SceneEditorTool::Drag);
	}
}

void SceneEditorWindow::setSaveEnabled(bool enabled)
{
	auto button = getWidgetAs<UIButton>("saveButton");
	button->setLabel(LocalisedString::fromHardcodedString(enabled ? "* Save" : "Save"));
	button->setEnabled(enabled);
}
