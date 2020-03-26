#pragma once
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

	protected:
		void update(Time t, bool moved) override;
		
	private:
		UIFactory& uiFactory;
		Project& project;

		std::shared_ptr<SceneEditorCanvas> canvas;

		String sceneName;
		EntityId sceneId;

		void makeUI();
		void load();
	};
}
