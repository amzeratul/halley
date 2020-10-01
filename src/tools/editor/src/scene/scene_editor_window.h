#pragma once
#include "entity_editor.h"
#include "entity_list.h"
#include "halley/ui/ui_widget.h"
#include "scene_editor_canvas.h"
#include "halley/tools/dll/dynamic_library.h"

namespace Halley {
	class SceneEditorTabs;
	class HalleyAPI;
	class Project;
	class UIFactory;
	class EntityFactory;

	class SceneEditorWindow final : public UIWidget, public IDynamicLibraryListener, public ISceneEditorWindow {
	public:
		SceneEditorWindow(UIFactory& factory, Project& project, const HalleyAPI& api, SceneEditorTabs& sceneEditorTabs);
		~SceneEditorWindow();

		void onAddedToRoot() override;
		
		void loadScene(const String& sceneName);
		void loadPrefab(const String& name);
		void loadScene(AssetType type, const Prefab& prefab);
		void unloadScene();
		void markModified() override;

		void onEntityAdded(const String& id, const String& parentId, const String& afterSiblingId) override;
		void onEntityRemoved(const String& id, const String& parentId) override;
		void onEntityModified(const String& id) override;
		void onEntityMoved(const String& id) override;
		void onComponentRemoved(const String& name) override;
		void onFieldChangedByGizmo(const String& componentName, const String& fieldName) override;

		void addNewEntity();
		void addNewPrefab();
		void addNewPrefab(const String& prefabName);
		void addEntity(ConfigNode data);
		void addEntity(const String& referenceEntityId, bool childOfReference, ConfigNode data);
		void addEntity(const String& parentId, const String& afterSibling, ConfigNode data);
		void removeEntity();
		void removeEntity(const String& entityId) override;
		void selectEntity(const String& id);
		void selectEntity(const std::vector<UUID>& candidates);

		void setTool(SceneEditorTool tool);
		void setTool(SceneEditorTool tool, const String& componentName, const String& fieldName, ConfigNode options);

		std::shared_ptr<const Prefab> getGamePrefab(const String& id) const;

		void copyEntityToClipboard(const String& id);
		void pasteEntityFromClipboard(const String& referenceId);
		String copyEntity(const String& id);
		void pasteEntity(const String& data, const String& referenceId);
		void duplicateEntity(const String& id);
		void openEditPrefabWindow(const String& name);

		const std::shared_ptr<ISceneData>& getSceneData() const override;

	protected:
		void update(Time t, bool moved) override;

		bool onKeyPress(KeyboardKeyPress key) override;

		void onUnloadDLL() override;
		void onLoadDLL() override;

	private:
		const HalleyAPI& api;
		UIFactory& uiFactory;
		Project& project;
		SceneEditorTabs& sceneEditorTabs;

		std::shared_ptr<SceneEditorGameBridge> gameBridge;
		std::shared_ptr<SceneEditorCanvas> canvas;
		std::shared_ptr<EntityList> entityList;
		std::shared_ptr<EntityEditor> entityEditor;
		std::shared_ptr<UIList> toolMode;

		Path assetPath;
		std::shared_ptr<ISceneData> sceneData;
		std::shared_ptr<Prefab> prefab;
		AssetType origPrefabAssetType;
		std::shared_ptr<EntityFactory> entityFactory;
		std::optional<EntityScene> currentEntityScene;

		String currentEntityId;

		std::shared_ptr<UIWidget> curCustomUI;
		std::shared_ptr<UIWidget> curToolUI;
		SceneEditorTool curTool = SceneEditorTool::None;
		String curComponentName;

		int toolModeTimeout = 0; // Hack

		void makeUI();
		void onEntitySelected(const String& id);
		void panCameraToEntity(const String& id);
		void saveEntity();

		String findParent(const String& entityId) const;
		const String* findParent(const String& entityId, const EntityTree& tree, const String& prev) const;

		void preparePrefab(Prefab& prefab);
		void preparePrefabEntity(ConfigNode& node);

		void setCustomUI(std::shared_ptr<UIWidget> ui);
		void setToolUI(std::shared_ptr<UIWidget> ui);

		void decayTool();

		void setSaveEnabled(bool enabled);

		String serializeEntity(const ConfigNode& node) const;
		std::optional<ConfigNode> deserializeEntity(const String& data) const;

		void assignUUIDs(ConfigNode& node);
		bool isValidEntityTree(const ConfigNode& node) const;

		void toggleConsole();
		void setupConsoleCommands();
	};
}
