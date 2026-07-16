#pragma once

#include <halley.hpp>

#include "localisation_filters.h"

namespace Halley {
	class LocalisationFiltersController {
	public:
		LocalisationFiltersController(UIWidget& ui, LocalisationFilters& filters);
		void setup();

	private:
		UIWidget& ui;
		LocalisationFilters& filters;

		void setPriorityEnabled(bool enabled);
		void setTranslatedEnabled(bool enabled);
		void setOutdatedEnabled(bool enabled);
		void setReadyEnabled(bool enabled);
		void setCommentEnabled(bool enabled);
		void setContextEnabled(bool enabled);
	};

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

		LocalisationFiltersController filterController;

		void applyFilters();
	};
}
