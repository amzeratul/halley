#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class UIFactory;

	class EntityList final : public UIWidget {
	public:
		EntityList(String id, UIFactory& factory);

	private:
		UIFactory& factory;

		void makeUI();
	};
}
