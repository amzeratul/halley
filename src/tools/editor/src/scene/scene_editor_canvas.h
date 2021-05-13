#pragma once
#include "halley/core/game/scene_editor_interface.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class SceneEditorGameBridge;
	class SceneEditorWindow;
	class UIFactory;
	class RenderSurface;
	class SceneEditorGizmoCollection;

	class SceneEditorCanvas final : public UIWidget {
	public:
		SceneEditorCanvas(String id, UIFactory& factory, Resources& resources, const HalleyAPI& api, std::optional<UISizer> sizer = {});

		void setGameBridge(SceneEditorGameBridge& gameBridge);
		void setSceneEditorWindow(SceneEditorWindow& editorWindow);

		std::shared_ptr<UIWidget> setTool(const String& tool, const String& componentName, const String& fieldName);

	protected:
		void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;
		void render(RenderContext& rc) const override;

		bool canInteractWithMouse() const override;
		bool isFocusLocked() const override;

		void pressMouse(Vector2f mousePos, int button) override;
		void releaseMouse(Vector2f mousePos, int button) override;
		void onMouseOver(Vector2f mousePos) override;
		void onMouseWheel(const UIEvent& event);

	private:
		UIFactory& factory;
		Resources& resources;
		SceneEditorWindow* editorWindow = nullptr;
		SceneEditorGameBridge* gameBridge = nullptr;

		Sprite border;

		std::shared_ptr<RenderSurface> surface;

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
	};
}
