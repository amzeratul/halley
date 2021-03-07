#pragma once
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class UIGraphNode : public UIWidget {
	public:
		UIGraphNode(UIFactory& factory);
	};
}
