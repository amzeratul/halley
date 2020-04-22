#pragma once
#include "halley/tools/dll/dynamic_library.h"

namespace Halley {
	class SceneEditorGameBridge {
	public:
		SceneEditorGameBridge(const HalleyAPI& api, Resources& resources, UIFactory& factory);
		~SceneEditorGameBridge();

		void update(Time t, SceneEditorInputState inputState, SceneEditorOutputState& outputState);
		void render(RenderContext& rc) const;

		void loadGame(std::shared_ptr<DynamicLibrary> dll, Resources& gameResources);
		bool needsReload() const;
		void reload();

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
		void onToolSet(SceneEditorTool tool, const String& componentName, const String& fieldName, ConfigNode options);

	private:
		const HalleyAPI& api;
		Resources& resources;
		
		std::shared_ptr<DynamicLibrary> gameDLL;
		std::unique_ptr<ISceneEditor> interface;
		std::unique_ptr<HalleyAPI> gameAPI;
		std::unique_ptr<CoreAPIInternal> gameCoreAPI;

		std::unique_ptr<SceneEditorGizmoCollection> gizmos;

		Resources* gameResources = nullptr;

		mutable bool errorState = false;
		bool interfaceReady = false;

		void loadDLL();
		void unloadDLL();
		void reloadDLL();

		void guardedRun(const std::function<void()>& f, bool allowFailure = false) const;
	};
}
