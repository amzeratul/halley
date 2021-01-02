#pragma once
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class TaskDetails : public UIWidget {
	public:
		TaskDetails(UIFactory& factory);

	private:
		UIFactory& factory;
	};
}
