#pragma once
#include "choose_asset_window.h"
#include "entity_editor.h"
#include "entity_list.h"
#include "halley/ui/ui_widget.h"
#include "scene_editor_canvas.h"
#include "halley/tools/dll/dynamic_library.h"
#include "undo_stack.h"
#include "halley/tools/dll/project_dll.h"

namespace Halley {
	class ProjectWindow;
	class AssetBrowserTabs;
	class HalleyAPI;
	class Project;
	class UIFactory;
	class EntityFactory;

	class SceneEditorWindow final : public UIWidget, public IProjectDLLListener, public ISceneEditorWindow {
	public:
		SceneEditorWindow(UIFactory& factory, Project& project, const HalleyAPI& api, ProjectWindow& projectWindow);
		~SceneEditorWindow();

		void onAddedToRoot(UIRoot& root) override;
		
		void loadScene(const String& sceneName);
		void loadPrefab(const String& name);
		void loadScene(AssetType type, const Prefab& prefab);
		void unloadScene();

		void onComponentRemoved(const String& name) override;
		void onFieldChangedByGizmo(const String& componentName, const String& fieldName);

		void addNewEntity(std::optional<String> reference = {}, bool childOfReference = false);
		void addNewPrefab(std::optional<String> reference = {}, bool childOfReference = false);
		std::optional<EntityData> makeInstance(const String& prefab) const;
		void addNewPrefab(const String& referenceEntityId, bool childOfReference, const String& prefabName);
		void addEntity(const String& referenceEntityId, bool childOfReference, EntityData data);
		void addEntities(const String& referenceEntityId, bool childOfReference, std::vector<EntityData> data);
		void removeSelectedEntities();
		void removeEntities(gsl::span<const String> entityIds) override;
		void replaceEntity(const String& entityId, EntityData newData);
		void selectEntity(const String& id, UIList::SelectionMode mode = UIList::SelectionMode::Normal);
		void selectEntities(gsl::span<const String> ids, UIList::SelectionMode mode = UIList::SelectionMode::Normal);
		void moveEntities(gsl::span<const EntityChangeOperation> changes, bool refreshEntityList = true);

		void onEntityModified(const String& id, const EntityData& prevData, const EntityData& newData) override;
		void onEntitiesModified(gsl::span<const String> ids, gsl::span<const EntityData*> prevDatas, gsl::span<const EntityData*> newData) override;

		void addEntities(gsl::span<const EntityChangeOperation> patches);
		void removeEntities(gsl::span<const EntityChangeOperation> patches);
		void modifyEntities(gsl::span<const EntityChangeOperation> patches);
		void replaceEntities(gsl::span<const EntityChangeOperation> patches);

		void extractPrefab(const String& id);
		void extractPrefab(const String& id, const String& prefabName);
		void collapsePrefab(const String& id);

		void setTool(String tool);
		void setTool(String tool, String componentName, String fieldName);

		std::shared_ptr<const Prefab> getGamePrefab(const String& id) const;

		void copyEntitiesToClipboard(gsl::span<const String> ids);
		void cutEntitiesToClipboard(gsl::span<const String> ids);
		void pasteEntitiesFromClipboard(const String& referenceId, bool childOfReference);
		String copyEntities(gsl::span<const String> ids);
		void pasteEntities(const String& data, const String& referenceId, bool childOfReference);
		void duplicateEntities(gsl::span<const String> ids);
		void openEditPrefabWindow(const String& name);

		const std::shared_ptr<ISceneData>& getSceneData() const override;

		void markModified() override;
		void clearModifiedFlag();
		bool isModified() const;
		void saveScene();

		const EntityIcons& getEntityIcons() const;

		void refreshAssets();

		void addComponentToCurrentEntity(const String& componentName) override;
		void setHighlightedComponents(std::vector<String> componentNames) override;
		const IEntityEditorFactory& getEntityEditorFactory() const override;

		std::shared_ptr<ScriptNodeTypeCollection> getScriptNodeTypes() override;
		
		const ConfigNode& getSetting(EditorSettingType type, std::string_view id) const override;
		void setSetting(EditorSettingType type, std::string_view id, ConfigNode data) override;

		Path getPrimaryInputFile(AssetType type, const String& assetId, bool absolute) const override;

		String getCurrentAssetId() const override;
		
		void onTabbedIn();

		float getProjectDefaultZoom() const override;

		std::shared_ptr<EntityFactory> getEntityFactory() const override;
		void spawnUI(std::shared_ptr<UIWidget> ui) override;
		
		void openAsset(AssetType assetType, const String& assetId);
		void openAssetHere(AssetType assetType, const String& assetId);
		String getAssetKey() const;

		std::vector<AssetCategoryFilter> getPrefabCategoryFilters() const;
		
		Future<AssetPreviewData> getAssetPreviewData(AssetType assetType, const String& id, Vector2i size);

		void onEntityContextMenuAction(const String& actionId, gsl::span<const String> entityIds);
		bool canPasteEntity() const;
		bool canAddSibling(const String& entityId) const;
		bool isPrefabInstance(const String& entityId) const;

		EntityValidator& getEntityValidator();
		void refreshGizmos();
		void validateAllEntities();

		ProjectWindow& getProjectWindow() const;

	protected:
		void update(Time t, bool moved) override;

		bool onKeyPress(KeyboardKeyPress key) override;

		void onProjectDLLStatusChange(ProjectDLL::Status status) override;

		void onEntitiesAdded(gsl::span<const EntityChangeOperation> patches);
		void onEntitiesRemoved(gsl::span<const String> ids, gsl::span<const std::pair<String, int>> parents, gsl::span<EntityData> prevDatas);
		void onEntityReplaced(const String& id, const String& parentId, int childIndex, const EntityData& prevData, const EntityData& newData);

	private:		
		const HalleyAPI& api;
		UIFactory& uiFactory;
		Project& project;
		ProjectWindow& projectWindow;

		std::shared_ptr<SceneEditorGameBridge> gameBridge;
		std::shared_ptr<SceneEditorCanvas> canvas;
		std::shared_ptr<EntityList> entityList;
		std::shared_ptr<EntityEditor> entityEditor;
		std::shared_ptr<EntityEditorFactory> entityEditorFactory;
		std::shared_ptr<UIList> toolMode;
		std::shared_ptr<EntityIcons> entityIcons;

		Path assetPath;
		std::shared_ptr<ISceneData> sceneData;
		std::shared_ptr<Prefab> prefab;
		AssetType origPrefabAssetType;
		std::shared_ptr<EntityFactory> entityFactory;
		std::optional<EntityScene> currentEntityScene;

		std::shared_ptr<EntityValidator> entityValidator;

		std::vector<String> currentEntityIds;

		std::shared_ptr<UIWidget> curCustomUI;
		std::shared_ptr<UIWidget> curToolUI;
		String curTool;
		String curComponentName;

		int toolModeTimeout = 1; // Hack

		UndoStack undoStack;
		bool modified = false;
		bool buttonsNeedUpdate = false;
		
		size_t assetReloadCallbackIdx = 0;
		std::vector<std::function<bool(gsl::span<const String>)>> assetReloadCallbacks;

		void makeUI();
		void onEntitiesSelected(std::vector<String> selectedEntities);
		void panCameraToEntity(const String& id);

		std::optional<String> findParent(const String& entityId, std::set<String>* invalidParents = nullptr) const;
		const String* findParent(const String& targetEntityId, const EntityTree& tree, const String& parentEntityId, std::set<String>* invalidParents) const;
		std::vector<std::pair<String, std::optional<String>>> findUniqueParents(gsl::span<const String> entityIds) const;

		String getNextSibling(const String& parentId, int childIndex) const;
		std::pair<String, int> getParentInsertPos(const String& referenceId, bool childOfReference) const;

		void setCustomUI(std::shared_ptr<UIWidget> ui);
		void setToolUI(std::shared_ptr<UIWidget> ui);

		void setModified(bool enabled);

		String serializeEntities(gsl::span<const EntityData> node) const;
		std::vector<EntityData> deserializeEntities(const String& data) const;

		void assignUUIDs(EntityData& node);
		void positionEntity(EntityData& entityData, Vector2f pos) const;
		Vector2f getEntityPosition(const EntityData& entityData) const;
		Vector2f getPositionClosestToAverage(gsl::span<const EntityData> datas) const;
		bool isValidEntityTree(const ConfigNode& node) const;

		void toggleConsole();
		void setupConsoleCommands();

		void updateButtons();

		void undo();
		void redo();
	};
}
