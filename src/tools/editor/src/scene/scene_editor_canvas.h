#pragma once
#include "halley/core/game/scene_editor_interface.h"
#include "halley/tools/dll/dynamic_library.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class SceneEditorWindow;
	class UIFactory;
	class RenderSurface;
	class SceneEditorGizmoCollection;

	class SceneEditorCanvas final : public UIWidget {
	public:
		SceneEditorCanvas(String id, UIFactory& factory, Resources& resources, const HalleyAPI& api, std::optional<UISizer> sizer = {});
		~SceneEditorCanvas();

		void loadGame(std::shared_ptr<DynamicLibrary> dll, Resources& gameResources);
		bool needsReload() const;
		void reload();

		bool isLoaded() const;
		ISceneEditor& getInterface() const;
		void setSceneEditorWindow(SceneEditorWindow& editorWindow);

		void guardedRun(const std::function<void()>& f) const;
		std::shared_ptr<UIWidget> setTool(SceneEditorTool tool, const String& componentName, const String& fieldName, const ConfigNode& options);

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
		const HalleyAPI& api;
		Resources& resources;
		Resources* gameResources = nullptr;
		SceneEditorWindow* editorWindow;

		Sprite border;

		std::shared_ptr<DynamicLibrary> gameDLL;
		std::unique_ptr<ISceneEditor> interface;
		std::unique_ptr<HalleyAPI> gameAPI;
		std::unique_ptr<CoreAPIInternal> gameCoreAPI;
		mutable bool errorState = false;

		std::shared_ptr<RenderSurface> surface;

		std::unique_ptr<SceneEditorGizmoCollection> gizmos;
		std::shared_ptr<InputKeyboard> keyboard;
		std::shared_ptr<InputDevice> mouse;
		SceneEditorInputState inputState;
		SceneEditorOutputState outputState;
		SceneEditorTool tool = SceneEditorTool::None;

		bool dragging = false;
		int dragButton = 0;
		Vector2f lastMousePos;

		void updateInterface(Time t);
		void renderInterface(RenderContext& rc) const;

		void loadDLL();
		void unloadDLL();
		void reloadDLL();

		void updateInputState();
		void notifyOutputState();
		void clearInputState();
	};
}
