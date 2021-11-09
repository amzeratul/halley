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
#include "halley/entity/components/transform_2d_component.h"
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

	project.withDLL([&] (ProjectDLL& dll)
	{
		dll.addReloadListener(*this);
	});

	assetReloadCallbackIdx = project.addAssetReloadCallback([=] (gsl::span<const String> assets)
	{
		std_ex::erase_if(assetReloadCallbacks, [&] (auto& callback) { return callback(assets); });
	});
}

SceneEditorWindow::~SceneEditorWindow()
{
	project.removeAssetReloadCallback(assetReloadCallbackIdx);
	
	unloadScene();

	entityEditor.reset();
	entityIcons.reset();
	
	project.withDLL([&] (ProjectDLL& dll)
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

	const auto validatorList = getWidgetAs<EntityValidatorListUI>("entityValidatorListUI");
	entityList->setEntityValidatorList(validatorList);
	validatorList->setList(entityList);

	entityEditor = getWidgetAs<EntityEditor>("entityEditor");
	entityEditor->setSceneEditorWindow(*this, api);

	toolMode = getWidgetAs<UIList>("toolMode");
	
	setModified(false);

	setHandle(UIEventType::ListSelectionChanged, "entityList_list", [=] (const UIEvent& event)
	{
		onEntitiesSelected(entityList->getCurrentSelections());
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
		removeSelectedEntities();
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
		const auto assetData = Path::readFile(project.getAssetsSrcPath() / assetPath);
		Scene scene;
		scene.parseYAML(gsl::as_bytes(gsl::span<const Byte>(assetData)));
		scene.setAssetId(name);
		loadScene(AssetType::Scene, scene);
		//loadScene(AssetType::Scene, *project.getGameResources().get<Scene>(name));
	}
}

void SceneEditorWindow::loadPrefab(const String& name)
{
	unloadScene();
	assetPath = project.getImportAssetsDatabase().getPrimaryInputFile(AssetType::Prefab, name);

	if (!name.isEmpty()) {
		const auto assetData = Path::readFile(project.getAssetsSrcPath() / assetPath);
		Prefab prefab;
		prefab.parseYAML(gsl::as_bytes(gsl::span<const Byte>(assetData)));
		prefab.setAssetId(name);
		loadScene(AssetType::Prefab, prefab);
		// loadScene(AssetType::Prefab, *project.getGameResources().get<Prefab>(name));
	}
}

void SceneEditorWindow::loadScene(AssetType assetType, const Prefab& origPrefab)
{
	if (sceneData) {
		unloadScene();
	}
	
	gameBridge->initializeInterfaceIfNeeded(true);
	if (gameBridge->isLoaded()) {
		auto& interface = gameBridge->getInterface();
		auto& world = interface.getWorld();

		// Entity validator
		entityValidator = std::make_shared<EntityValidator>(world);
		gameBridge->initializeEntityValidator(*entityValidator);

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
		entityEditor->setSceneEditorWindow(*this, api);
		entityEditor->setECSData(project.getECSData());
		entityEditor->setEntityEditorFactory(entityEditorFactory);
		entityList->setSceneData(sceneData);

		// Select entity
		const auto& initialEntities = projectWindow.getAssetSetting(getAssetKey(), "currentEntities");
		if (initialEntities.getType() != ConfigNodeType::Undefined) {
			selectEntities(initialEntities.asVector<String>());
		}

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
	setToolUI({});

	currentEntityIds = {};
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
	entityEditor->unloadEntity(false);
	entityEditor->setEntityEditorFactory({});
	entityEditor->unloadIcons();
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
			currentEntityScene->update(*entityFactory, nullptr);
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
	if (key.is(KeyCode::Z, KeyMods::Ctrl)) {
		undo();
		return true;
	}

	if (key.is(KeyCode::Y, KeyMods::Ctrl) || key.is(KeyCode::Z, KeyMods::CtrlShift)) {
		redo();
		return true;
	}
	
	if (key.is(KeyCode::Delete)) {
		removeSelectedEntities();
		return true;
	}

	if (key.is(KeyCode::C, KeyMods::Ctrl)) {
		copyEntitiesToClipboard(entityList->getCurrentSelections());
		return true;
	}

	if (key.is(KeyCode::X, KeyMods::Ctrl)) {
		cutEntitiesToClipboard(entityList->getCurrentSelections());
		return true;
	}

	if (key.is(KeyCode::V, KeyMods::Ctrl)) {
		pasteEntitiesFromClipboard(entityList->getCurrentSelection(), false);
		return true;
	}

	if (key.is(KeyCode::D, KeyMods::Ctrl)) {
		duplicateEntities(entityList->getCurrentSelections());
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

	if (key.is(KeyCode::F2)) {
		entityEditor->focusRenameEntity();
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

void SceneEditorWindow::onEntityContextMenuAction(const String& actionId, gsl::span<const String> entityIds)
{
	if (actionId == "copy") {
		copyEntitiesToClipboard(entityIds);
	} else if (actionId == "cut") {
		cutEntitiesToClipboard(entityIds);
	} else if (actionId == "paste_sibling") {
		pasteEntitiesFromClipboard(entityIds.front(), false);
	} else if (actionId == "paste_child") {
		pasteEntitiesFromClipboard(entityIds.front(), true);
	} else if (actionId == "delete") {
		removeEntities(entityIds);
	} else if (actionId == "duplicate") {
		duplicateEntities(entityIds);
	} else if (actionId == "add_entity_child") {
		addNewEntity(entityIds.front(), true);
	} else if (actionId == "add_entity_sibling") {
		addNewEntity(entityIds.front(), false);
	} else if (actionId == "add_prefab_child") {
		addNewPrefab(entityIds.front(), true);
	} else if (actionId == "add_prefab_sibling") {
		addNewPrefab(entityIds.front(), false);
	} else if (actionId == "extract_prefab") {
		extractPrefab(entityIds.front());
	} else if (actionId == "collapse_prefab") {
		collapsePrefab(entityIds.front());
	}
}

bool SceneEditorWindow::canPasteEntity() const
{
	const auto clipboard = api.system->getClipboard();
	if (clipboard) {
		const auto clipboardData = clipboard->getStringData();
		if (clipboardData) {
			const auto data = deserializeEntities(clipboardData.value());
			return !data.empty();
		}
	}
	return false;
}

bool SceneEditorWindow::canAddSibling(const String& entityId) const
{
	const auto& ref = sceneData->getEntityNodeData(entityId);
	return prefab->isScene() || !ref.getParentId().isEmpty();
}

bool SceneEditorWindow::isPrefabInstance(const String& entityId) const
{
	const auto& ref = sceneData->getEntityNodeData(entityId);
	return !ref.getData().getPrefab().isEmpty();
}

EntityValidator& SceneEditorWindow::getEntityValidator()
{
	return *entityValidator;
}

void SceneEditorWindow::refreshGizmos()
{
	gameBridge->getGizmos().refreshEntity();
}

void SceneEditorWindow::validateAllEntities()
{
	entityList->validateAllEntities();
}

void SceneEditorWindow::onProjectDLLStatusChange(ProjectDLL::Status status)
{
	if (status == ProjectDLL::Status::Loaded) {
		if (prefab) {
			loadScene(origPrefabAssetType, *prefab);
		}
	} else {
		unloadScene();
	}
}

void SceneEditorWindow::selectEntity(const String& id, UIList::SelectionMode mode)
{
	entityList->select(id, mode);
}

void SceneEditorWindow::selectEntities(gsl::span<const String> ids, UIList::SelectionMode mode)
{
	entityList->select(ids, mode);
}

void SceneEditorWindow::addEntity(const String& referenceEntity, bool childOfReference, EntityData data)
{
	addEntities(referenceEntity, childOfReference, std::vector<EntityData>({ std::move(data) }));
}

void SceneEditorWindow::addEntities(const String& referenceEntity, bool childOfReference, std::vector<EntityData> datas)
{
	if (datas.empty()) {
		return;
	}

	const auto parentInfo = getParentInsertPos(referenceEntity, childOfReference);
	const auto& parentId = parentInfo.first;
	int childPos = parentInfo.second;

	// Find the placement position
	// This is based on the anchor closest to the average pos, and the cursor pos
	const Vector2f targetPos = getPositionClosestToAverage(datas);
	const Vector2f basePos = gameBridge->getMousePos().value_or(gameBridge->getCameraPos()).round() - targetPos;

	// Find the relevant transform
	const Transform2DComponent identity;
	const Transform2DComponent* transform = parentId.isEmpty() ? &identity : gameBridge->getTransform(parentId);
	if (!transform) {
		transform = &identity;
	}

	std::vector<EntityChangeOperation> ops;
	for (auto& data: datas) {
		positionEntity(data, transform->inverseTransformPoint(basePos + getEntityPosition(data)));

		const auto& id = data.getInstanceUUID().toString();
		ops.emplace_back(EntityChangeOperation{ std::make_unique<EntityData>(std::move(data)), id, parentId, childPos});
		if (childPos >= 0) {
			++childPos;
		}
	}
	addEntities(ops);
}

void SceneEditorWindow::addEntities(gsl::span<const EntityChangeOperation> changes)
{
	if (changes.empty()) {
		return;
	}

	for (const auto& change: changes) {
		EntityData& parentData = sceneData->getWriteableEntityNodeData(change.parent).getData();
		if (parentData.getPrefab().isEmpty() && (change.parent != "" || parentData.isSceneRoot())) {
			const size_t insertPos = change.childIndex >= 0 ? static_cast<size_t>(change.childIndex) : std::numeric_limits<size_t>::max();
			auto& seq = parentData.getChildren();

			seq.insert(seq.begin() + std::min(insertPos, seq.size()), change.data->asEntityData());
		}
	}

	onEntitiesAdded(changes);
}

void SceneEditorWindow::onEntitiesAdded(gsl::span<const EntityChangeOperation> changes)
{
	for (const auto& change: changes) {
		const auto& id = change.data->asEntityData().getInstanceUUID().toString();
		const auto& parentId = change.parent;
		sceneData->reloadEntity(parentId.isEmpty() ? id : parentId);
	}

	entityList->onEntitiesAdded(changes);
	onEntitiesSelected(entityList->getCurrentSelections());
	undoStack.pushAdded(modified, changes);
	
	markModified();
}

void SceneEditorWindow::removeEntities(gsl::span<const EntityChangeOperation> patches)
{
	std::vector<String> ids;
	ids.reserve(patches.size());
	for (const auto& p: patches) {
		ids.push_back(p.entityId);
	}
	removeEntities(ids);
}

void SceneEditorWindow::modifyEntities(gsl::span<const EntityChangeOperation> patches)
{
	std::vector<String> ids;
	std::vector<EntityData> oldDatas;
	std::vector<const EntityData*> oldDataPtrs;
	std::vector<const EntityData*> newDataPtrs;
	ids.reserve(ids.size());
	oldDatas.reserve(patches.size());
	oldDataPtrs.reserve(patches.size());
	newDataPtrs.reserve(patches.size());
	
	for (const auto& patch: patches) {
		auto& data = sceneData->getWriteableEntityNodeData(patch.entityId).getData();
		ids.emplace_back(patch.entityId);
		oldDatas.emplace_back(EntityData(data));
		oldDataPtrs.emplace_back(&oldDatas.back()); // This is only OK because of the reserve above, otherwise the pointer might be invalidated
		newDataPtrs.emplace_back(&data);
		data.applyDelta(patch.data->asEntityDataDelta());
	}
	
	onEntitiesModified(ids, oldDataPtrs, newDataPtrs);
	entityEditor->reloadEntity();
}

void SceneEditorWindow::moveEntities(gsl::span<const EntityChangeOperation> origChanges, bool refreshEntityList)
{
	std::vector<EntityChangeOperation> forward;
	std::vector<EntityChangeOperation> back;
	std::vector<String> ids;
	forward.reserve(origChanges.size());
	back.reserve(origChanges.size());
	ids.reserve(origChanges.size());

	for (auto& c: origChanges) {
		forward.push_back(c.clone());
	}

	for (auto& c: forward) {
		auto [prevParent, prevIndex] = sceneData->getEntityParenting(c.entityId);
		back.push_back(EntityChangeOperation{ {}, c.entityId, prevParent, static_cast<int>(prevIndex) });
		ids.push_back(c.entityId);
	}

	std::sort(forward.begin(), forward.end(), [&] (const EntityChangeOperation& a, const EntityChangeOperation& b)
	{
		if (a.parent != b.parent) {
			return a.parent < b.parent;
		}
		return a.childIndex < b.childIndex;
	});
	for (auto& c: forward) {
		sceneData->reparentEntity(c.entityId, c.parent, c.childIndex);
	}

	if (refreshEntityList) {
		entityList->refreshList();
	}

	// Not sure why this is all needed
	auto curSorted = currentEntityIds;
	std::sort(curSorted.begin(), curSorted.end());
	std::sort(ids.begin(), ids.end());
	if (!curSorted.empty() && curSorted == ids) {
		onEntitiesSelected(entityList->getCurrentSelections());
	}

	undoStack.pushMoved(modified, forward, back);
	
	markModified();
}

void SceneEditorWindow::replaceEntities(gsl::span<const EntityChangeOperation> patches)
{
	// TODO
	Expects(patches.size() == 1);
	replaceEntity(patches.front().entityId, EntityData(patches.front().data->asEntityData()));
}

void SceneEditorWindow::extractPrefab(const String& id)
{
	auto data = sceneData->getEntityNodeData(id);
	const auto basePath = project.getAssetsSrcPath() / "prefab";
	
	auto parameters = FileChooserParameters();
	parameters.fileName = data.getData().getName().replaceAll(" ", "_").asciiLower();
	parameters.defaultPath = basePath;
	parameters.fileTypes.emplace_back(FileChooserParameters::FileType{ "Halley Prefab", {"prefab"}, true });
	parameters.save = true;
	OS::get().openFileChooser(parameters).then([this, id, basePath] (std::optional<Path> path)
	{
		if (path) {
			if (path->getExtension() == ".prefab") {
				extractPrefab(id, path.value().makeRelativeTo(basePath).replaceExtension("").toString());
			}
		}
	});
}

void SceneEditorWindow::extractPrefab(const String& id, const String& prefabName)
{
	const auto entityNodeData = sceneData->getEntityNodeData(id);
	auto entityData = entityNodeData.getData();

	// Generate instance components and clear prefab
	auto components = std::vector<std::pair<String, ConfigNode>>();
	for (auto& c: entityData.getComponents()) {
		if (c.first == "Transform2D" || c.first == "Transform3D") {
			components.emplace_back(c);
			c.second = ConfigNode::MapType();
		}
	}
	
	// This callback will be invoked once the assets are imported
	assetReloadCallbacks.push_back([this, prefabName, id, components] (gsl::span<const String> assetIds) -> bool
	{
		if (!std_ex::contains(assetIds, "prefab:" + prefabName)) {
			return false;
		}

		// Collect data
		const auto entityNodeData = sceneData->getEntityNodeData(id);
		const auto uuid = entityNodeData.getData().getInstanceUUID();

		// Replace entity
		EntityData instanceData;
		instanceData.setInstanceUUID(uuid);
		instanceData.setPrefab(prefabName);
		instanceData.setComponents(components);
		replaceEntity(id, std::move(instanceData));

		return true;
	});
	
	// Write prefab
	const auto serializedData = serializeEntities(gsl::span<const EntityData>(&entityData, 1));
	project.writeAssetToDisk(Path("prefab") / (prefabName + ".prefab"), serializedData);
}

void SceneEditorWindow::collapsePrefab(const String& id)
{
	// Collect data
	const auto entityNodeData = sceneData->getEntityNodeData(id);
	const auto prefabName = entityNodeData.getData().getPrefab();

	// Get prefab
	if (prefabName.isEmpty()) {
		return;
	}
	const auto prefab = project.getGameResources().get<Prefab>(prefabName);

	// Update entity
	EntityData instanceData = prefab->getEntityData().instantiateWithAsCopy(entityNodeData.getData());
	replaceEntity(id, std::move(instanceData));
}

void SceneEditorWindow::onEntitiesSelected(std::vector<String> selectedEntities)
{
	try {
		if (selectedEntities.size() == 1) {
			const auto& entityId = selectedEntities.front();
			auto& firstEntityData = sceneData->getWriteableEntityNodeData(entityId).getData();
			const Prefab* prefabData = nullptr;
			const String prefabName = firstEntityData.getPrefab();
			if (!prefabName.isEmpty()) {
				prefabData = getGamePrefab(prefabName).get();
			}
			entityEditor->loadEntity(entityId, firstEntityData, prefabData, false, project.getGameResources());
		} else {
			entityEditor->unloadEntity(!selectedEntities.empty());
		}

		std::vector<UUID> uuids;
		std::vector<EntityData*> datas = sceneData->getWriteableEntityDatas(selectedEntities);
		uuids.reserve(selectedEntities.size());
		for (const auto& id: selectedEntities) {
			uuids.push_back(UUID(id));
		}
		gameBridge->setSelectedEntities(std::move(uuids), std::move(datas));
		
		currentEntityIds = selectedEntities;
		projectWindow.setAssetSetting(getAssetKey(), "currentEntities", ConfigNode(currentEntityIds));
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
	project.setAssetSaveNotification(false);
	project.writeAssetToDisk(assetPath, gsl::as_bytes(gsl::span<const char>(strData.c_str(), strData.length())));
	gameBridge->onSceneSaved();
	project.setAssetSaveNotification(true);
}

void SceneEditorWindow::markModified()
{
	setModified(true);
}

void SceneEditorWindow::clearModifiedFlag()
{
	setModified(false);
}

void SceneEditorWindow::onEntityModified(const String& id, const EntityData& prevData, const EntityData& newData)
{
	const auto* oldPtr = &prevData;
	const auto* newPtr = &newData;
	onEntitiesModified(gsl::span<const String>(&id, 1), gsl::span<const EntityData*>(&oldPtr, 1), gsl::span<const EntityData*>(&newPtr, 1));
}

void SceneEditorWindow::onEntitiesModified(gsl::span<const String> ids, gsl::span<const EntityData*> prevDatas, gsl::span<const EntityData*> newDatas)
{
	Expects(ids.size() == prevDatas.size());
	Expects(ids.size() == newDatas.size());

	if (undoStack.pushModified(modified, ids, prevDatas, newDatas)) {
		for (size_t i = 0; i < ids.size(); ++i) {
			sceneData->reloadEntity(ids[i], newDatas[i]);
			entityList->onEntityModified(ids[i], *newDatas[i]);
		}
		markModified();
	}
}

void SceneEditorWindow::onEntityReplaced(const String& id, const String& parentId, int childIndex, const EntityData& prevData, const EntityData& newData)
{
	if (!id.isEmpty()) {
		const bool hadChange = undoStack.pushReplaced(modified, id, prevData, newData);

		if (hadChange) {
			sceneData->reloadEntity(id);

			const auto op = EntityChangeOperation{ std::make_unique<EntityData>(newData), id, parentId, childIndex };
			entityList->onEntitiesRemoved(gsl::span<const String>(&id, 1), "");
			entityList->onEntitiesAdded(gsl::span<const EntityChangeOperation>(&op, 1));

			markModified();
		}
	}
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

void SceneEditorWindow::copyEntitiesToClipboard(gsl::span<const String> ids)
{
	const auto clipboard = api.system->getClipboard();
	if (clipboard) {
		clipboard->setData(copyEntities(ids));
	}
}

void SceneEditorWindow::cutEntitiesToClipboard(gsl::span<const String> ids)
{
	copyEntitiesToClipboard(ids);
	removeEntities(ids);
}

void SceneEditorWindow::pasteEntitiesFromClipboard(const String& referenceId, bool childOfReference)
{
	const auto clipboard = api.system->getClipboard();
	if (clipboard) {
		auto clipboardData = clipboard->getStringData();
		if (clipboardData) {
			pasteEntities(clipboardData.value(), referenceId, childOfReference);
		}
	}
}

String SceneEditorWindow::copyEntities(gsl::span<const String> ids)
{
	std::vector<EntityData> datas;

	for (const auto& [id, parentId]: findUniqueParents(ids)) {
		if (!parentId) {
			continue;
		}
		auto data = sceneData->getEntityNodeData(id).getData();

		if (const auto* transform = gameBridge->getTransform(id)) {
			positionEntity(data, transform->getGlobalPosition());
		}

		datas.push_back(std::move(data));
	}
	return serializeEntities(datas);
}

void SceneEditorWindow::pasteEntities(const String& stringData, const String& referenceId, bool childOfReference)
{
	Expects(gameBridge);
	auto datas = deserializeEntities(stringData);
	if (!datas.empty()) {
		for (auto& data: datas) {
			assignUUIDs(data);
		}
		addEntities(referenceId, childOfReference, std::move(datas));
	}
}

void SceneEditorWindow::duplicateEntities(gsl::span<const String> ids)
{
	if (!ids.empty()) {
		pasteEntities(copyEntities(ids), ids.back(), false);
	}
}

void SceneEditorWindow::openEditPrefabWindow(const String& name)
{
	projectWindow.openAsset(AssetType::Prefab, name, true);
}

const std::shared_ptr<ISceneData>& SceneEditorWindow::getSceneData() const
{
	return sceneData;
}

void SceneEditorWindow::addNewEntity(std::optional<String> reference, bool childOfReference)
{
	EntityData data;
	data.setInstanceUUID(UUID::generate());
	data.getComponents().emplace_back("Transform2D", ConfigNode::MapType());
	addEntity(reference.value_or(currentEntityIds.empty() ? "" : currentEntityIds.front()), childOfReference, std::move(data));
}

void SceneEditorWindow::addNewPrefab(std::optional<String> reference, bool childOfReference)
{
	getRoot()->addChild(std::make_shared<ChoosePrefabWindow>(uiFactory, "", project.getGameResources(), *this, [=] (std::optional<String> result)
	{
		if (result) {
			addNewPrefab(reference.value_or(currentEntityIds.empty() ? "" : currentEntityIds.front()), childOfReference, result.value());
		}
	}));
}

void SceneEditorWindow::addNewPrefab(const String& referenceEntityId, bool childOfReference, const String& prefabName)
{
	auto data = makeInstance(prefabName);
	if (data) {
		addEntity(referenceEntityId, childOfReference, std::move(data.value()));
	}
}

std::optional<EntityData> SceneEditorWindow::makeInstance(const String& prefabName) const
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
		return data;
	}

	return {};
}

std::pair<String, int> SceneEditorWindow::getParentInsertPos(const String& referenceEntity, bool childOfReference) const
{
	if (referenceEntity.isEmpty()) {
		return { "", -1 };
	} else {
		const bool isScene = prefab->isScene();
		
		const auto& ref = sceneData->getEntityNodeData(referenceEntity);
		const bool canBeSibling = !ref.getParentId().isEmpty() || isScene;
		const bool canBeChild = ref.getData().getPrefab().isEmpty();
		if (!canBeChild && !canBeSibling) {
			return { "", -1 };
		}
		
		const bool addAsChild = (childOfReference && canBeChild) || !canBeSibling;
		
		if (addAsChild) {
			return { referenceEntity, -1 };
		} else {
			const String& parentId = ref.getParentId();
			const auto& parentRef = sceneData->getEntityNodeData(parentId);
			const auto idx = parentRef.getData().getChildIndex(UUID(referenceEntity));
			const int childIndex = idx ? static_cast<int>(idx.value() + 1) : -1;
			return { parentId, childIndex };
		}
	}
}

void SceneEditorWindow::removeSelectedEntities()
{
	removeEntities(currentEntityIds);
}

void SceneEditorWindow::removeEntities(gsl::span<const String> targetIds)
{
	if (targetIds.empty()) {
		return;
	}

	std::vector<String> entityIds;
	std::vector<std::pair<String, int>> parenting;
	std::vector<EntityData> prevDatas;
	std::vector<String> toClean;
	
	for (const auto& [targetId, parentId]: findUniqueParents(targetIds)) {
		if (!parentId) {
			continue;
		}

		auto& parentData = sceneData->getWriteableEntityNodeData(parentId.value()).getData();

		// Don't delete root of prefab
		const auto canDelete = !parentId.value().isEmpty() || parentData.isSceneRoot();
		if (!canDelete) {
			continue;
		}

		auto& children = parentData.getChildren();
		for (auto iter = children.begin(); iter != children.end(); ++iter) {
			if (iter->getInstanceUUID().toString() == targetId) {
				entityIds.push_back(targetId);
				prevDatas.emplace_back(std::move(*iter));
				parenting.emplace_back(parentId.value(), static_cast<int>(iter - children.begin()));

				// Mark it empty, will swipe later and delete those
				// We don't want to delete them right now or it'll affect the index of other entities being deleted
				iter->setInstanceUUID(UUID());
				toClean.push_back(parentId.value());

				break;
			}
		}
	}

	for (auto& id: toClean) {
		auto& datas = sceneData->getWriteableEntityNodeData(id).getData();
		std_ex::erase_if(datas.getChildren(), [] (const EntityData& data) { return !data.getInstanceUUID().isValid(); });
	}

	onEntitiesRemoved(entityIds, parenting, prevDatas);
}

void SceneEditorWindow::onEntitiesRemoved(gsl::span<const String> ids, gsl::span<const std::pair<String, int>> parents, gsl::span<EntityData> prevDatas)
{
	Expects(ids.size() == parents.size());
	Expects(ids.size() == prevDatas.size());
	if (ids.empty()) {
		return;
	}

	undoStack.pushRemoved(modified, ids, parents, prevDatas);

	const String& newSelectionId = getNextSibling(parents.front().first, parents.front().second);
	entityList->onEntitiesRemoved(ids, newSelectionId);

	for (size_t i = 0; i < ids.size(); ++i) {
		sceneData->reloadEntity(parents[i].first.isEmpty() ? ids[i] : parents[i].first);
	}
	onEntitiesSelected(entityList->getCurrentSelections());

	markModified();
}

void SceneEditorWindow::replaceEntity(const String& entityId, EntityData newData)
{
	auto node = sceneData->getWriteableEntityNodeData(entityId);
	auto oldData = node.getData();
	node.getData() = EntityData(newData);
	onEntityReplaced(entityId, node.getParentId(), node.getChildIndex(), oldData, newData);
}

std::optional<String> SceneEditorWindow::findParent(const String& entityId, std::set<String>* invalidParents) const
{
	const String rootId = "";
	const auto tree = sceneData->getEntityTree();
	const auto res = findParent(entityId, tree, rootId, invalidParents);
	return res ? *res : std::optional<String>();
}

const String* SceneEditorWindow::findParent(const String& targetEntityId, const EntityTree& tree, const String& parentEntityId, std::set<String>* invalidParents) const
{
	if (tree.entityId == targetEntityId) {
		return &parentEntityId;
	}
	
	if (!invalidParents || !std_ex::contains(*invalidParents, tree.entityId)) {
		for (auto& c: tree.children) {
			const auto res = findParent(targetEntityId, c, tree.entityId, invalidParents);
			if (res) {
				return res;
			}
		}
	}

	return nullptr;
}

std::vector<std::pair<String, std::optional<String>>> SceneEditorWindow::findUniqueParents(gsl::span<const String> entityIds) const
{
	std::vector<std::pair<String, std::optional<String>>> result;
	std::set<String> idSet;
	for (const auto& entityId: entityIds) {
		idSet.insert(entityId);
	}
	for (const auto& entityId: entityIds) {
		result.emplace_back(entityId, findParent(entityId, &idSet));
	}
	return result;
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

	// Attempt to reload missing prefabs
	
}

void SceneEditorWindow::addComponentToCurrentEntity(const String& componentName)
{
	entityEditor->addComponent(componentName, ConfigNode::MapType());
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

String SceneEditorWindow::serializeEntities(gsl::span<const EntityData> nodes) const
{
	YAMLConvert::EmitOptions options;
	options.mapKeyOrder = {{ "name", "icon", "prefab", "uuid", "components", "children" }};

	ConfigNode result;
	if (nodes.size() == 1) {
		result = nodes.front().toConfigNode(false);
	} else {
		auto seq = ConfigNode::SequenceType();
		for (auto& node: nodes) {
			seq.emplace_back(node.toConfigNode(false));
		}
		result = std::move(seq);
	}

	return YAMLConvert::generateYAML(result, options);
}

std::vector<EntityData> SceneEditorWindow::deserializeEntities(const String& data) const
{
	ConfigFile file;
	try {
		YAMLConvert::parseConfig(file, gsl::as_bytes(gsl::span<const char>(data.c_str(), data.length())));
		std::vector<EntityData> result;
		if (file.getRoot().getType() == ConfigNodeType::Sequence) {
			for (auto& n: file.getRoot().asSequence()) {
				if (!isValidEntityTree(n)) {
					return {};
				}
				result.emplace_back(n, false);
			}
		} else {
			if (!isValidEntityTree(file.getRoot())) {
				return {};
			}
			result.emplace_back(file.getRoot(), false);
		}
		return result;
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

void SceneEditorWindow::positionEntity(EntityData& entityData, Vector2f pos) const
{
	for (auto& [componentName, component]: entityData.getComponents()) {
		if (componentName == "Transform2D") {
			component.asMap()["position"] = pos;
		}
	}
}

Vector2f SceneEditorWindow::getEntityPosition(const EntityData& entityData) const
{
	for (auto& [componentName, component]: entityData.getComponents()) {
		if (componentName == "Transform2D") {
			return component["position"].asVector2f(Vector2f());
		}
	}
	return Vector2f();
}

Vector2f SceneEditorWindow::getPositionClosestToAverage(gsl::span<const EntityData> datas) const
{
	Vector2f averagePos;
	for (const auto& data: datas) {
		averagePos += getEntityPosition(data);
	}
	averagePos /= static_cast<float>(datas.size());
	float bestDist = std::numeric_limits<float>::infinity();
	Vector2f bestPos;
	for (const auto& data: datas) {
		const auto pos = getEntityPosition(data);
		const auto dist = (pos - averagePos).length();
		if (dist < bestDist) {
			bestDist = dist;
			bestPos = pos;
		}
	}
	return bestPos;
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
	refreshGizmos();
	updateButtons();
}

void SceneEditorWindow::redo()
{
	undoStack.redo(*this);
	refreshGizmos();
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

Path SceneEditorWindow::getPrimaryInputFile(AssetType type, const String& assetId, bool absolute) const
{
	auto path = project.getImportAssetsDatabase().getPrimaryInputFile(type, assetId);
	if (absolute) {
		return project.getAssetsSrcPath() / path;
	} else {
		return path;
	}
}

String SceneEditorWindow::getCurrentAssetId() const
{
	return prefab->getAssetId();
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
	projectWindow.openAsset(assetType, assetId, true);
}

void SceneEditorWindow::openAssetHere(AssetType assetType, const String& assetId)
{
	projectWindow.replaceAssetTab(origPrefabAssetType, prefab->getAssetId(), assetType, assetId);
}

String SceneEditorWindow::getAssetKey() const
{
	return origPrefabAssetType + ":" + prefab->getAssetId();
}

std::vector<AssetCategoryFilter> SceneEditorWindow::getPrefabCategoryFilters() const
{
	return gameBridge->getPrefabCategoryFilters();
}

Future<AssetPreviewData> SceneEditorWindow::getAssetPreviewData(AssetType assetType, const String& id, Vector2i size)
{
	auto cached = project.getCachedAssetPreview(assetType, id);
	if (cached) {
		// Convert image to sprite
		auto data = std::move(cached.value());
		data.sprite.setImage(project.getGameResources(), *api.video, data.image);
		return Future<AssetPreviewData>::makeImmediate(std::move(data));
	}

	return gameBridge->getAssetPreviewData(assetType, id, size).then([=] (AssetPreviewData data) -> AssetPreviewData
	{
		// Store in cache
		project.setCachedAssetPreview(assetType, id, data);
		
		// Convert image to sprite
		data.sprite.setImage(project.getGameResources(), *api.video, data.image);
		return data;
	});
}
