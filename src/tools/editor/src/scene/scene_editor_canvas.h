#pragma once
#include "halley/tools/dll/dynamic_library.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class SceneEditorCanvas : public UIWidget {
	public:
		SceneEditorCanvas(String id, Resources& resources);
		~SceneEditorCanvas();

		void setGameDLL(std::shared_ptr<DynamicLibrary> dll);

	protected:
		void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;

	private:
		Resources& resources;

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
