#pragma once
#include "halley/tools/dll/dynamic_library.h"

namespace Halley {
	class Project;

	class SceneEditorGameBridge {
	public:
		SceneEditorGameBridge(const HalleyAPI& api, Resources& resources, UIFactory& factory, Project& project);
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
		void setSelectedEntity(const UUID& uuid, ConfigNode& data);
		void showEntity(const UUID& uuid);
		void onEntityAdded(const UUID& uuid, const ConfigNode& data);
		void onEntityRemoved(const UUID& uuid);
		void onEntityModified(const UUID& uuid, const ConfigNode& data);
		void onEntityMoved(const UUID& uuid, const ConfigNode& data);
		ConfigNode onToolSet(SceneEditorTool tool, const String& componentName, const String& fieldName, ConfigNode options);
		void onSceneLoaded(AssetType type, const String& assetId);
		void setupConsoleCommands(UIDebugConsoleController& controller);

	private:
		const HalleyAPI& api;
		Resources& resources;
		Project& project;
		
		std::unique_ptr<ISceneEditor> interface;
		std::unique_ptr<HalleyAPI> gameAPI;
		std::unique_ptr<CoreAPIInternal> gameCoreAPI;

		std::unique_ptr<SceneEditorGizmoCollection> gizmos;

		Resources* gameResources = nullptr;

		mutable bool errorState = false;
		bool interfaceReady = false;

		void load();

		void guardedRun(const std::function<void()>& f, bool allowFailure = false) const;
	};
}
