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

		void onEntityAdded(const String& id, const String& parentId, int childIndex) override;
		void onEntityRemoved(const String& id, const String& parentId, int childIndex, const EntityData& prevData) override;
		void onEntityModified(const String& id, const EntityData& prevData, const EntityData& newData) override;
		void onEntityMoved(const String& id, const String& prevParentId, int prevChildIndex, const String& newParentId, int newChildIndex) override;
		void onComponentRemoved(const String& name) override;
		void onFieldChangedByGizmo(const String& componentName, const String& fieldName) override;

		void addNewEntity(std::optional<String> reference = {}, bool childOfReference = false);
		void addNewPrefab(std::optional<String> reference = {}, bool childOfReference = false);
		void addNewPrefab(const String& referenceEntityId, bool childOfReference, const String& prefabName);
		void addEntity(const String& referenceEntityId, bool childOfReference, EntityData data);
		void addEntity(const String& parentId, int childIndex, EntityData data);
		void removeEntity();
		void removeEntity(const String& entityId) override;
		void selectEntity(const String& id);
		void modifyEntity(const String& id, const EntityDataDelta& delta);
		void moveEntity(const String& id, const String& newParent, int childIndex);

		void extractPrefab(const String& id);
		void collapsePrefab(const String& id);

		void setTool(String tool);
		void setTool(String tool, String componentName, String fieldName);

		std::shared_ptr<const Prefab> getGamePrefab(const String& id) const;

		void copyEntityToClipboard(const String& id);
		void cutEntityToClipboard(const String& id);
		void pasteEntityFromClipboard(const String& referenceId, bool childOfReference);
		String copyEntity(const String& id);
		void pasteEntity(const String& data, const String& referenceId, bool childOfReference);
		void duplicateEntity(const String& id);
		void openEditPrefabWindow(const String& name);

		const std::shared_ptr<ISceneData>& getSceneData() const override;

		void markModified() override;
		void clearModifiedFlag();
		bool isModified() const;
		const EntityIcons& getEntityIcons() const;

		void refreshAssets();

		void addComponentToCurrentEntity(const String& componentName) override;
		void setHighlightedComponents(std::vector<String> componentNames) override;
		const IEntityEditorFactory& getEntityEditorFactory() const override;

		std::shared_ptr<ScriptNodeTypeCollection> getScriptNodeTypes() override;
		
		const ConfigNode& getSetting(EditorSettingType type, std::string_view id) const override;
		void setSetting(EditorSettingType type, std::string_view id, ConfigNode data) override;

		void onTabbedIn();

		float getProjectDefaultZoom() const override;

		std::shared_ptr<EntityFactory> getEntityFactory() const override;
		void spawnUI(std::shared_ptr<UIWidget> ui) override;
		
		void openAsset(AssetType assetType, const String& assetId);
		void openAssetHere(AssetType assetType, const String& assetId);
		String getAssetKey() const;

		std::vector<AssetCategoryFilter> getPrefabCategoryFilters() const;
		
		Future<AssetPreviewData> getAssetPreviewData(AssetType assetType, const String& id, Vector2i size);

		void onEntityContextMenuAction(const String& actionId, const String& entityId);
		bool canPasteEntity() const;
		bool canAddSibling(const String& entityId) const;
		bool isPrefabInstance(const String& entityId) const;

	protected:
		void update(Time t, bool moved) override;

		bool onKeyPress(KeyboardKeyPress key) override;

		void onProjectDLLStatusChange(ProjectDLL::Status status) override;
	
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

		String currentEntityId;

		std::shared_ptr<UIWidget> curCustomUI;
		std::shared_ptr<UIWidget> curToolUI;
		String curTool;
		String curComponentName;

		int toolModeTimeout = 1; // Hack

		UndoStack undoStack;
		bool modified = false;
		bool buttonsNeedUpdate = false;

		void makeUI();
		void onEntitySelected(const String& id);
		void panCameraToEntity(const String& id);
		void saveScene();

		String findParent(const String& entityId) const;
		const String* findParent(const String& entityId, const EntityTree& tree, const String& prev) const;
		String getNextSibling(const String& parentId, int childIndex) const;

		void setCustomUI(std::shared_ptr<UIWidget> ui);
		void setToolUI(std::shared_ptr<UIWidget> ui);

		void setModified(bool enabled);

		String serializeEntity(const EntityData& node) const;
		std::optional<EntityData> deserializeEntity(const String& data) const;

		void assignUUIDs(EntityData& node);
		void positionEntityAtCursor(EntityData& entityData) const;
		void positionEntity(EntityData& entityData, Vector2f pos) const;
		bool isValidEntityTree(const ConfigNode& node) const;

		void toggleConsole();
		void setupConsoleCommands();

		void updateButtons();

		void undo();
		void redo();
	};
}
