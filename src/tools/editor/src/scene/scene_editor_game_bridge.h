#pragma once
#include "halley/core/game/scene_editor_interface.h"
#include "halley/tools/dll/dynamic_library.h"

namespace Halley {
	class SceneEditorWindow;
	class ProjectWindow;
	class Project;

	class SceneEditorGameBridge : private IEditorInterface {
	public:
		SceneEditorGameBridge(const HalleyAPI& api, Resources& resources, UIFactory& factory, Project& project, ProjectWindow& projectWindow, SceneEditorWindow& sceneEditorWindow);
		~SceneEditorGameBridge();

		void unload();

		void update(Time t, SceneEditorInputState inputState, SceneEditorOutputState& outputState);
		void render(RenderContext& rc) const;

		bool isLoaded() const;
		ISceneEditor& getInterface() const;
		void initializeInterfaceIfNeeded(bool force);

		SceneEditorGizmoCollection& getGizmos() const;

		void changeZoom(int amount, Vector2f mousePos);
		void dragCamera(Vector2f pos);
		void moveCamera(Vector2f pos);
		bool loadCameraPos();
		std::shared_ptr<UIWidget> makeCustomUI() const;
		void setSelectedEntity(const UUID& uuid, EntityData& data);
		void setEntityHighlightedOnList(const UUID& uuid);
		void showEntity(const UUID& uuid);
		void onEntityAdded(const UUID& uuid, const EntityData& data);
		void onEntityRemoved(const UUID& uuid);
		void onEntityModified(const UUID& uuid, const EntityData& oldData, const EntityData& newData);
		void onEntityMoved(const UUID& uuid, const EntityData& data);
		void onToolSet(String& tool, String& componentName, String& fieldName);
		void onSceneLoaded(Prefab& scene);
		void onSceneSaved();
		void setupConsoleCommands(UIDebugConsoleController& controller, ISceneEditorWindow& sceneEditor);
		void cycleHighlight(int delta);
		std::optional<Vector2f> getMousePos() const;
		Vector2f getCameraPos() const;

		void refreshAssets();

		std::shared_ptr<ScriptNodeTypeCollection> getScriptNodeTypes();

		std::vector<UIPopupMenuItem> getSceneContextMenu(const Vector2f& mousePos) const;
		void onSceneContextMenuSelection(const String& id);
		void onSceneContextMenuHighlight(const String& id);

	protected:
		bool saveAsset(const Path& path, gsl::span<const gsl::byte> data) override;
		void openAsset(AssetType assetType, const String& assetId) override;
		void addTask(std::unique_ptr<Task> task) override;
		
		const ConfigNode& getSetting(EditorSettingType type, std::string_view id) const override;
		void setSetting(EditorSettingType type, std::string_view id, ConfigNode data) override;
		const ConfigNode& getAssetSetting(std::string_view id) const override;
		void setAssetSetting(std::string_view id, ConfigNode data) override;

		void selectEntity(const String& uuid) override;

	private:
		const HalleyAPI& api;
		Resources& resources;
		Project& project;
		ProjectWindow& projectWindow;
		UIFactory& factory;
		SceneEditorWindow& sceneEditorWindow;
		
		std::unique_ptr<ISceneEditor> interface;
		std::unique_ptr<HalleyAPI> gameAPI;
		std::unique_ptr<CoreAPIInternal> gameCoreAPI;

		std::unique_ptr<SceneEditorGizmoCollection> gizmos;

		Resources* gameResources = nullptr;

		std::map<Path, Bytes> pendingAssets;

		mutable bool errorState = false;
		bool interfaceInitializationError = false;
		bool interfaceReady = false;

		void load();

		bool guardedRun(const std::function<void()>& f, bool allowFailure = false) const;
	};
}
