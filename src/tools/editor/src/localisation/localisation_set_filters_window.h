#pragma once

#include <halley.hpp>

#include "localisation_filters.h"

namespace Halley {
	class LocalisationSetFiltersWindow : public UIWidget {
	public:
		using Callback = std::function<void(bool)>;
		LocalisationSetFiltersWindow(UIFactory& factory, LocalisationFilters& filters, Vector2f pos, Callback callback);

		void onMakeUI() override;

	private:
		UIFactory& factory;
		LocalisationFilters& originalFilters;
		LocalisationFilters workingCopy;
		Callback callback;

		void loadFilters();
		void applyFilters();
	};
}
