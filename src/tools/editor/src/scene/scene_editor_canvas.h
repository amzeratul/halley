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

	private:
		const HalleyAPI& api;
		Resources& resources;
		Resources* gameResources = nullptr;

		Sprite border;
		Sprite canvas;

		std::shared_ptr<DynamicLibrary> gameDLL;
		std::unique_ptr<SceneEditorInterface> interface;

		void updateInterface(Time t);
		void renderInterface() const;

		void loadDLL();
		void unloadDLL();
		void reloadDLL();
	};
}
