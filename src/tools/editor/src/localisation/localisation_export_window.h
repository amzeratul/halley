#pragma once

#include <halley.hpp>

#include "localisation_set_filters_window.h"

namespace Halley {
	class LocalisationExportWindow : public UIWidget {
	public:
		using Callback = std::function<void(bool, LocalisationFilters)>;
		LocalisationExportWindow(UIFactory& factory, Callback callback);

		void onMakeUI() override;

	private:
		UIFactory& factory;
		LocalisationFilters filters;
		Callback callback;

		LocalisationFiltersController filterController;
	};
}
