#pragma once
#include "halley/game/scene_editor_interface.h"
#include "halley/tools/dll/project_dll.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class SceneEditorGameBridge;
	class SceneEditorWindow;
	class UIFactory;
	class RenderSurface;
	class SceneEditorGizmoCollection;

	class SceneEditorCanvas final : public UIWidget, IProjectDLLListener {
	public:
		SceneEditorCanvas(String id, UIFactory& factory, Resources& resources, const HalleyAPI& api, Project& project, std::optional<UISizer> sizer = {});
		~SceneEditorCanvas();

		void setGameBridge(SceneEditorGameBridge& gameBridge);
		void setSceneEditorWindow(SceneEditorWindow& editorWindow);

		std::shared_ptr<UIWidget> setTool(const String& tool, const String& componentName, const String& fieldName);

	protected:
		void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;
		void render(RenderContext& rc) const override;

		bool canInteractWithMouse() const override;
		bool isFocusLocked() const override;
		bool canReceiveFocus() const override;

		void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
		void releaseMouse(Vector2f mousePos, int button) override;
		void onMouseOver(Vector2f mousePos) override;
		void onMouseWheel(const UIEvent& event);
		
		void onProjectDLLStatusChange(ProjectDLL::Status status) override;

	private:
		UIFactory& factory;
		Resources& resources;
		Project& project;

		SceneEditorWindow* editorWindow = nullptr;
		SceneEditorGameBridge* gameBridge = nullptr;

		Sprite border;

		std::shared_ptr<RenderSurface> surface;
		bool ready = false;
		int frameN = 0;

		std::shared_ptr<InputKeyboard> keyboard;
		std::shared_ptr<InputDevice> mouse;
		SceneEditorInputState inputState;
		SceneEditorOutputState outputState;
		String tool;

		bool dragging = false;
		int dragButton = 0;
		Vector2f lastMousePos;

		void updateInputState();
		void notifyOutputState();
		void clearInputState();

		void openRightClickMenu();

		Vector2i getCanvasSize() const;
	};
}
