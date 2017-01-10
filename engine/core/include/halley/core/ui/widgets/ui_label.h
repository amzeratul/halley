#pragma once
#include "ui/ui_widget.h"
#include "graphics/text/text_renderer.h"

namespace Halley {
	class UILabel : public UIWidget {
	public:
		explicit UILabel(TextRenderer text);

	private:
		TextRenderer text;
	};
}
