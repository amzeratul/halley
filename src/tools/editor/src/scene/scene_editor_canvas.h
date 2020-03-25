#pragma once
#include "halley/tools/dll/dynamic_library.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class SceneEditorCanvas : public UIWidget {
	public:
		SceneEditorCanvas(String id, Resources& resources, const HalleyAPI& api);
		~SceneEditorCanvas();

		void setGameDLL(std::shared_ptr<DynamicLibrary> dll, Resources& gameResources);
		bool needsReload() const;
		void reload();

	protected:
		void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;
		void render(RenderContext& rc) const override;
		
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

		std::shared_ptr<TextureRenderTarget> renderTarget;
		Vector2i curTextureSize;
		Vector2i curRenderSize;

		void updateInterface(Time t);
		void renderInterface(RenderContext& rc) const;
		void updateCanvas(Vector2i size);

		void loadDLL();
		void unloadDLL();
		void reloadDLL();

		void guardedRun(const std::function<void()>& f) const;
	};
}
