#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class HalleyEditor;
	class UIFactory;

	class LoadProjectWindow final : public UIWidget
	{
	public:
		LoadProjectWindow(UIFactory& factory, HalleyEditor& editor, std::function<void(String)> callback);

		void onAddedToRoot() override;

	protected:
		void update(Time t, bool moved) override;

	private:
		HalleyEditor& editor;
	};
}
