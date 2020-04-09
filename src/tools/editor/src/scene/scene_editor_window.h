#pragma once
#include "entity_editor.h"
#include "entity_list.h"
#include "halley/ui/ui_widget.h"
#include "scene_editor_canvas.h"

namespace Halley {
	class HalleyAPI;
	class Project;
	class UIFactory;

	class SceneEditorWindow final : public UIWidget {
	public:
		SceneEditorWindow(UIFactory& factory, Project& project, const HalleyAPI& api);
		~SceneEditorWindow();

		void loadScene(const String& sceneName);
		void unloadScene();
		void markModified();
		void modifyEntity(const String& id, const ConfigNode& data);

	protected:
		void update(Time t, bool moved) override;
		
	private:
		UIFactory& uiFactory;
		Project& project;

		std::shared_ptr<SceneEditorCanvas> canvas;
		std::shared_ptr<EntityList> entityList;
		std::shared_ptr<EntityEditor> entityEditor;

		String sceneName;
		EntityId sceneId;
		std::shared_ptr<ISceneData> sceneData;
		std::unique_ptr<Prefab> prefab;
		std::shared_ptr<EntityFactory> entityFactory;

		String currentEntityId;

		void makeUI();
		void load();
		void selectEntity(const String& id);
		void panCameraToEntity(const String& id);
		void saveEntity();
	};
}
