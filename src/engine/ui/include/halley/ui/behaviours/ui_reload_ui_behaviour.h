#pragma once

#include "../ui_behaviour.h"
#include "halley/file_formats/config_file.h"

namespace Halley {
	class ConfigObserver;
	class UIFactory;

	class UIReloadUIBehaviour final : public UIBehaviour {
	public:
		UIReloadUIBehaviour(UIFactory& factory, ConfigObserver observer);

		void update(Time time) override;
		
	private:
		UIFactory& factory;
		ConfigObserver observer;
	};
}
