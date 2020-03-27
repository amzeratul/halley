#pragma once
#include "halley/tools/dll/dynamic_library.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class SceneEditorCanvas final : public UIWidget {
	public:
		SceneEditorCanvas(String id, Resources& resources, const HalleyAPI& api);
		~SceneEditorCanvas();

		void loadGame(std::shared_ptr<DynamicLibrary> dll, Resources& gameResources);
		bool needsReload() const;
		void reload();

		bool isLoaded() const;
		SceneEditorInterface& getInterface() const;

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

		Sprite border;

		std::shared_ptr<DynamicLibrary> gameDLL;
		std::unique_ptr<SceneEditorInterface> interface;
		std::unique_ptr<HalleyAPI> gameAPI;
		std::unique_ptr<CoreAPIInternal> gameCoreAPI;
		mutable bool errorState = false;

		std::shared_ptr<RenderSurface> surface;

		bool dragging = false;
		Vector2f lastMousePos;

		void updateInterface(Time t);
		void renderInterface(RenderContext& rc) const;

		void loadDLL();
		void unloadDLL();
		void reloadDLL();

		void guardedRun(const std::function<void()>& f) const;
	};
}
