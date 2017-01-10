#pragma once
#include "ui_element.h"

namespace Halley {
	class UIRoot : public IUIParent {
	public:
		UIRoot& getRoot() override;
	};
}
