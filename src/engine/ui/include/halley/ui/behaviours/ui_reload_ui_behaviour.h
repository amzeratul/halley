#pragma once

#include "../ui_behaviour.h"
#include "halley/file_formats/config_file.h"

namespace Halley {
	class ConfigObserver;
	class UIFactory;
	class UIStyleSheet;

	class UIReloadUIBehaviour final : public UIBehaviour {
	public:
		UIReloadUIBehaviour(UIFactory& factory, ResourceObserver observer);

		void init() override;
		void update(Time time) override;
		
	private:
		UIFactory& factory;
		ResourceObserver observer;
		std::vector<std::pair<String, int>> uiStyleObservers;

		bool requireStyleUpdate() const;
		bool styleObserverNeedsUpdate(const std::pair<String, int>& observer) const;

		void setupUIStyleObservers();

		void getStyleObservers(const UIWidget& widget, const UIStyleSheet& stylesheet, std::vector<std::pair<String, int>>& styleObservers) const;
	};
}
