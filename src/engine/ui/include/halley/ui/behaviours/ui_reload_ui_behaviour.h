#pragma once

#include "../ui_behaviour.h"
#include "halley/file_formats/config_file.h"

namespace Halley {
	class ConfigObserver;
	class UIFactory;
	class UILabel;

	class UIReloadUIBehaviour final : public UIBehaviour {
	public:
		UIReloadUIBehaviour(UIFactory& factory, ConfigObserver observer);

		void init() override;
		void update(Time time) override;
		
	private:
		UIFactory& factory;
		ConfigObserver observer;
		std::vector<std::pair<String, int>> uiStyleObservers;

		bool requireStyleUpdate() const;
		bool styleObserverNeedsUpdate(const std::pair<String, int>& observer) const;

		void setupUIStyleObservers();

		void getStyleObservers(const std::shared_ptr<UIWidget>& widget, std::vector<std::pair<String, int>>& styleObservers) const;
	};
}
