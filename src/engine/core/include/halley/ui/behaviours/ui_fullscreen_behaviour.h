#pragma once

#include "../ui_behaviour.h"

namespace Halley {
	class UIAnchor;

	class UIFullscreenBehaviour final : public UIBehaviour {
	public:
		void onAddedToRoot(UIRoot& root) override;
		void update(Time time) override;
	};
}
