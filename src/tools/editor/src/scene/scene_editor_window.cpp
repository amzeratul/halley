#include "scene_editor_window.h"



#include "entity_editor.h"
#include "entity_list.h"
#include "halley/tools/file/filesystem.h"
#include "halley/tools/project/project.h"
#include "halley/tools/yaml/yaml_convert.h"
#include "scene_editor_canvas.h"
using namespace Halley;

SceneEditorWindow::SceneEditorWindow(UIFactory& factory, Project& project, const HalleyAPI& api)
	: UIWidget("scene_editor", {}, UISizer())
	, uiFactory(factory)
	, project(project)
{
	makeUI();
	load();
}

SceneEditorWindow::~SceneEditorWindow()
{
	unloadScene();
}

void SceneEditorWindow::loadScene(const String& name)
{
	Expects(canvas);
	
	unloadScene();
	
	if (!name.isEmpty() && canvas->isLoaded()) {
		sceneName = name;
		auto& interface = canvas->getInterface();
		auto& world = interface.getWorld();

		// Load prefab
		prefab = std::make_unique<Prefab>(*project.getGameResources().get<Prefab>(name));
		preparePrefab(*prefab);

		// Spawn scene
		entityFactory = std::make_shared<EntityFactory>(world, project.getGameResources());
		auto entities = entityFactory->createScene(prefab->getRoot());
		interface.spawnPending();

		// Setup editors
		sceneData = std::make_shared<PrefabSceneData>(*prefab, entityFactory, world, project.getGameResources());
		entityEditor->setECSData(project.getECSData());
		entityEditor->addFieldFactories(interface.getComponentEditorFieldFactories());
		entityList->setSceneData(sceneData);

		// HACK: set to drag tool
		interface.setTool(SceneEditorTool::Translate);

		// Show root
		if (!entities.empty()) {
			panCameraToEntity(entities.at(0).getUUID().toString());
		}		
	}
}

void SceneEditorWindow::unloadScene()
{
	Expects(canvas);
	
	if (canvas->isLoaded()) {
		auto& interface = canvas->getInterface();
		auto& world = interface.getWorld();
		const auto cameraId = interface.getCameraId();
		for (auto& e: world.getTopLevelEntities()) {
			if (e.getEntityId() != cameraId) {
				world.destroyEntity(e);
			}
		}		
		world.spawnPending();
	}
	sceneName = "";
	entityFactory.reset();
	sceneData.reset();
}

void SceneEditorWindow::update(Time t, bool moved)
{
	if (canvas->needsReload()) {
		const String name = sceneName;
		unloadScene();
		canvas->reload();
		loadScene(name);
	}
}

void SceneEditorWindow::makeUI()
{
	add(uiFactory.makeUI("ui/halley/scene_editor_window"), 1);
	
	canvas = getWidgetAs<SceneEditorCanvas>("canvas");
	canvas->setSceneEditorWindow(*this);
	
	entityList = getWidgetAs<EntityList>("entityList");
	entityList->setSceneEditorWindow(*this);

	entityEditor = getWidgetAs<EntityEditor>("entityEditor");
	entityEditor->setSceneEditorWindow(*this);

	getWidget("saveButton")->setEnabled(false);

	setHandle(UIEventType::ListSelectionChanged, "entityList_list", [=] (const UIEvent& event)
	{
		selectEntity(event.getStringData());
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
		addEntity();
	});

	setHandle(UIEventType::ButtonClicked, "removeEntity", [=] (const UIEvent& event)
	{
		removeEntity();
	});
}

void SceneEditorWindow::load()
{
	const auto& dll = project.getGameDLL();
	if (dll) {
		canvas->loadGame(dll, project.getGameResources());
	}
}

void SceneEditorWindow::selectEntity(const String& id)
{
	String actualId = id;
	if (actualId.isEmpty()) {
		const auto& tree = sceneData->getEntityTree();
		if (tree.entityId.isEmpty()) {
			if (tree.children.empty()) {
				ConfigNode empty;
				entityEditor->loadEntity("", empty, false);
				return;
			} else {
				actualId = tree.children[0].entityId;
			}
		} else {
			actualId = tree.entityId;
		}
	}

	auto& entityData = sceneData->getEntityData(actualId).data;
	const bool changed = entityEditor->loadEntity(actualId, entityData, false);
	if (changed) {
		canvas->getInterface().setSelectedEntity(UUID(actualId), entityData);
		currentEntityId = actualId;
	}
}

void SceneEditorWindow::panCameraToEntity(const String& id)
{
	canvas->getInterface().showEntity(UUID(id));
}

void SceneEditorWindow::saveEntity()
{
	getWidget("saveButton")->setEnabled(false);

	YAMLConvert::EmitOptions options;
	options.mapKeyOrder = {{ "name", "uuid", "components", "children" }};

	auto path = project.getAssetsSrcPath() / "prefab" / (sceneName + ".yaml");
	auto strData = YAMLConvert::generateYAML(*prefab, options);
	auto data = gsl::as_bytes(gsl::span<const char>(strData.c_str(), strData.length()));
	FileSystem::writeFile(path, data);
}

void SceneEditorWindow::markModified()
{
	getWidget("saveButton")->setEnabled(true);
}

void SceneEditorWindow::onEntityAdded(const String& id, const String& parentId)
{
	auto& data = sceneData->getEntityData(id).data;
	entityList->onEntityAdded(id, parentId, data);
	sceneData->reloadEntity(parentId.isEmpty() ? id : parentId);
	selectEntity(id);
	markModified();
}

void SceneEditorWindow::onEntityRemoved(const String& id, const String& parentId)
{
	entityList->onEntityRemoved(id, parentId);
	sceneData->reloadEntity(parentId.isEmpty() ? id : parentId);
	selectEntity(parentId);
	markModified();
}

void SceneEditorWindow::onEntityModified(const String& id)
{
	if (!id.isEmpty()) {
		entityList->onEntityModified(id, sceneData->getEntityData(id).data);
	}
	sceneData->reloadEntity(id);
	markModified();
}

void SceneEditorWindow::onFieldChangedByGizmo(const String& componentName, const String& fieldName)
{
	entityEditor->onFieldChangedByGizmo(componentName, fieldName);
	markModified();
}

void SceneEditorWindow::addEntity()
{
	if (!currentEntityId.isEmpty()) {
		const String& tryParentId = currentEntityId;
		const auto res = sceneData->getEntityData(tryParentId);
		const bool isParentPrefab = res.data.hasKey("prefab");
		const String& parentId = isParentPrefab ? res.parentId : tryParentId;
		addEntity(parentId);
	}
}

void SceneEditorWindow::addEntity(const String& parentId)
{
	const auto newId = UUID::generate().toString();

	auto newEntityData = ConfigNode(ConfigNode::MapType());
	newEntityData["name"] = "New Entity";
	newEntityData["uuid"] = newId;
	newEntityData["children"] = ConfigNode::SequenceType();
	newEntityData["components"] = ConfigNode::SequenceType();

	if (parentId.isEmpty()) {
		// Should only be able to place items on the root if it's a scene
		auto& root = sceneData->getEntityData("").data;
		root.asSequence().emplace_back(ConfigNode(newEntityData));
	} else {
		ConfigNode& parentData = sceneData->getEntityData(parentId).data;
		auto& children = parentData["children"];
		if (children.getType() != ConfigNodeType::Sequence) {
			children = ConfigNode::SequenceType();
		}
		children.asSequence().emplace_back(ConfigNode(newEntityData));
	}

	onEntityAdded(newId, parentId);
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
