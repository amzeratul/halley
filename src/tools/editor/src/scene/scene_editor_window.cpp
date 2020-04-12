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
		auto& interface = canvas->getInterface();
		auto& world = interface.getWorld();

		// Load prefab
		prefab = std::make_unique<Prefab>(*project.getGameResources().get<Prefab>(name));
		preparePrefab(*prefab);

		// Spawn scene
		entityFactory = std::make_shared<EntityFactory>(world, project.getGameResources());
		auto entity = entityFactory->createEntity(prefab->getRoot());
		interface.spawnPending();

		// Get scene data
		sceneId = entity.getEntityId();
		sceneName = name;

		// Setup editors
		sceneData = std::make_shared<PrefabSceneData>(*prefab, entityFactory, world);
		entityEditor->setECSData(project.getECSData());
		entityEditor->addFieldFactories(interface.getComponentEditorFieldFactories());
		entityList->setSceneData(sceneData);

		// HACK: set to drag tool
		interface.setTool(SceneEditorTool::Translate);

		// Show root
		panCameraToEntity(entity.getUUID().toString());
	}
}

void SceneEditorWindow::unloadScene()
{
	Expects(canvas);
	
	if (canvas->isLoaded() && sceneId.isValid()) {
		auto& world = canvas->getInterface().getWorld();
		world.destroyEntity(sceneId);
		world.spawnPending();
	}
	sceneName = "";
	sceneId = EntityId();
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
	auto& entityData = sceneData->getEntityData(id);
	const bool changed = entityEditor->loadEntity(id, entityData, false);
	if (changed) {
		canvas->getInterface().setSelectedEntity(UUID(id), entityData);
		currentEntityId = id;
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

void SceneEditorWindow::onEntityModified(const String& id)
{
	entityList->onEntityModified(id, sceneData->getEntityData(id));
	sceneData->reloadEntity(id);
	markModified();
}

void SceneEditorWindow::onFieldChangedByGizmo(const String& componentName, const String& fieldName)
{
	entityEditor->onFieldChangedByGizmo(componentName, fieldName);
}

void SceneEditorWindow::addEntity()
{
	if (!currentEntityId.isEmpty()) {
		const String& parentId = currentEntityId;

		const auto newId = UUID::generate().toString();
		
		auto& data = sceneData->getEntityData(parentId);
		auto newEntityData = ConfigNode(ConfigNode::MapType());
		newEntityData["name"] = "New Entity";
		newEntityData["uuid"] = newId;

		auto& children = data["children"];
		if (children.getType() != ConfigNodeType::Sequence) {
			children = ConfigNode::SequenceType();
		}
		// Store a copy (keep original for updates below)
		children.asSequence().emplace_back(ConfigNode(newEntityData));

		entityList->onEntityAdded(newId, parentId, newEntityData);
		onEntityModified(parentId);
		selectEntity(newId);
	}
}

void SceneEditorWindow::removeEntity()
{
	if (!currentEntityId.isEmpty()) {
		const String& targetId = currentEntityId;
		const String& parentId = findParent(currentEntityId);

		if (!parentId.isEmpty()) {
			auto& data = sceneData->getEntityData(parentId);
			auto& children = data["children"].asSequence();

			children.erase(std::remove_if(children.begin(), children.end(), [&] (const ConfigNode& child)
			{
				return child["uuid"].asString("") == targetId;
			}), children.end());

			entityList->onEntityRemoved(targetId, parentId);
			onEntityModified(parentId);
			selectEntity(parentId);
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

void SceneEditorWindow::preparePrefab(Prefab& prefab)
{
	preparePrefabEntity(prefab.getRoot());
}

void SceneEditorWindow::preparePrefabEntity(ConfigNode& node)
{
	if (!node.hasKey("name")) {
		node["name"] = "Entity";
	}
	
	if (!node.hasKey("uuid")) {
		node["uuid"] = UUID::generate().toString();
	}

	if (!node.hasKey("components")) {
		node["components"] = ConfigNode::SequenceType();
	}

	if (!node.hasKey("children")) {
		node["children"] = ConfigNode::SequenceType();
	} else {
		for (auto& c: node["children"].asSequence()) {
			preparePrefabEntity(c);
		}
	}
}
