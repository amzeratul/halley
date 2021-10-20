#pragma once

namespace Halley {
	class EntityValidatorUI : public UIWidget {
	public:
		EntityValidatorUI(UIFactory& factory);

	private:
		UIFactory& factory;
	};
}