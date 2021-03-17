#pragma once
#include "halley/core/game/scene_editor_interface.h"
#include "halley/tools/dll/dynamic_library.h"

namespace Halley {
	class ProjectWindow;
	class Project;

	class SceneEditorGameBridge : private IEditorInterface {
	public:
		SceneEditorGameBridge(const HalleyAPI& api, Resources& resources, UIFactory& factory, Project& project, ProjectWindow& projectWindow);
		~SceneEditorGameBridge();

		void unload();

		void update(Time t, SceneEditorInputState inputState, SceneEditorOutputState& outputState);
		void render(RenderContext& rc) const;

		bool isLoaded() const;
		ISceneEditor& getInterface() const;
		void initializeInterfaceIfNeeded();

		SceneEditorGizmoCollection& getGizmos() const;

		void changeZoom(int amount, Vector2f mousePos);
		void dragCamera(Vector2f pos);
		std::shared_ptr<UIWidget> makeCustomUI() const;
		void setSelectedEntity(const UUID& uuid, EntityData& data);
		void showEntity(const UUID& uuid);
		void onEntityAdded(const UUID& uuid, const EntityData& data);
		void onEntityRemoved(const UUID& uuid);
		void onEntityModified(const UUID& uuid, const EntityData& data);
		void onEntityMoved(const UUID& uuid, const EntityData& data);
		ConfigNode onToolSet(const String& tool, const String& componentName, const String& fieldName, ConfigNode options);
		void onSceneLoaded(Prefab& scene);
		void onSceneSaved();
		void setupConsoleCommands(UIDebugConsoleController& controller, ISceneEditorWindow& sceneEditor);

		void refreshAssets();

	protected:
		bool saveAsset(const Path& path, gsl::span<const gsl::byte> data) override;
		void addTask(std::unique_ptr<Task> task) override;
	
	private:
		const HalleyAPI& api;
		Resources& resources;
		Project& project;
		ProjectWindow& projectWindow;
		UIFactory& factory;
		
		std::unique_ptr<ISceneEditor> interface;
		std::unique_ptr<HalleyAPI> gameAPI;
		std::unique_ptr<CoreAPIInternal> gameCoreAPI;

		std::unique_ptr<SceneEditorGizmoCollection> gizmos;

		Resources* gameResources = nullptr;

		std::map<Path, Bytes> pendingAssets;

		mutable bool errorState = false;
		bool interfaceReady = false;

		void load();

		void guardedRun(const std::function<void()>& f, bool allowFailure = false) const;
	};
}
