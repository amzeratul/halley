#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class HalleyEditor;
	class UIFactory;

	class LoadProjectWindow : public UIWidget
	{
	public:
		LoadProjectWindow(UIFactory& factory, HalleyEditor& editor, std::function<void(const String&)> callback);
	};
}
